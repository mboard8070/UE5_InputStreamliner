// Copyright Epic Games, Inc. All Rights Reserved.

#include "LLMIntentParser.h"
#include "InputStreamlinerModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

ULLMIntentParser::ULLMIntentParser()
{
}

void ULLMIntentParser::SetEndpoint(const FString& URL, int32 Port)
{
	EndpointURL = URL;
	EndpointPort = Port;
}

void ULLMIntentParser::SetModel(const FString& InModelName)
{
	ModelName = InModelName;
}

void ULLMIntentParser::CheckConnection(FOnParseComplete OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(FString::Printf(TEXT("%s:%d/api/tags"), *EndpointURL, EndpointPort));
	Request->SetVerb(TEXT("GET"));
	Request->SetTimeout(5.0f);

	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			if (bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200)
			{
				OnComplete.ExecuteIfBound(true, TEXT(""));
			}
			else
			{
				FString Error = TEXT("Could not connect to Ollama. Is it running?");
				OnComplete.ExecuteIfBound(false, Error);
			}
		});

	Request->ProcessRequest();
}

void ULLMIntentParser::ParseInputDescriptionAsync(const FString& Description, FOnParseComplete OnComplete)
{
	if (bParseInProgress)
	{
		OnComplete.ExecuteIfBound(false, TEXT("Parse already in progress"));
		return;
	}

	bParseInProgress = true;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(FString::Printf(TEXT("%s:%d/api/generate"), *EndpointURL, EndpointPort));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetTimeout(60.0f); // LLM can take a while

	// Build request body
	TSharedPtr<FJsonObject> RequestBody = MakeShareable(new FJsonObject());
	RequestBody->SetStringField(TEXT("model"), ModelName);
	RequestBody->SetStringField(TEXT("prompt"), BuildPrompt(Description));
	RequestBody->SetBoolField(TEXT("stream"), false);

	// Set generation parameters for more consistent output
	TSharedPtr<FJsonObject> Options = MakeShareable(new FJsonObject());
	Options->SetNumberField(TEXT("temperature"), 0.1); // Low temperature for consistent output
	Options->SetNumberField(TEXT("top_p"), 0.9);
	RequestBody->SetObjectField(TEXT("options"), Options);

	FString RequestString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestString);
	FJsonSerializer::Serialize(RequestBody.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestString);

	Request->OnProcessRequestComplete().BindUObject(this, &ULLMIntentParser::OnHttpResponseReceived, OnComplete);

	UE_LOG(LogInputStreamliner, Log, TEXT("Sending parse request to LLM: %s"), *Description);
	Request->ProcessRequest();
}

void ULLMIntentParser::OnHttpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnParseComplete Callback)
{
	bParseInProgress = false;

	if (!bWasSuccessful || !Response.IsValid())
	{
		FString Error = TEXT("HTTP request failed");
		UE_LOG(LogInputStreamliner, Error, TEXT("%s"), *Error);
		Callback.ExecuteIfBound(false, Error);
		OnParseCompleted.Broadcast(false, Error);
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		FString Error = FString::Printf(TEXT("HTTP error: %d"), Response->GetResponseCode());
		UE_LOG(LogInputStreamliner, Error, TEXT("%s"), *Error);
		Callback.ExecuteIfBound(false, Error);
		OnParseCompleted.Broadcast(false, Error);
		return;
	}

	// Parse Ollama response
	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (!FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		FString Error = TEXT("Failed to parse Ollama response JSON");
		UE_LOG(LogInputStreamliner, Error, TEXT("%s"), *Error);
		Callback.ExecuteIfBound(false, Error);
		OnParseCompleted.Broadcast(false, Error);
		return;
	}

	// Extract the response field from Ollama's format
	FString ResponseText = JsonResponse->GetStringField(TEXT("response"));

	UE_LOG(LogInputStreamliner, Verbose, TEXT("LLM Response: %s"), *ResponseText);

	// Parse the JSON from the LLM response
	FString ParseError;
	if (!ParseJSONResponse(ResponseText, ParseError))
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to parse LLM output: %s"), *ParseError);
		Callback.ExecuteIfBound(false, ParseError);
		OnParseCompleted.Broadcast(false, ParseError);
		return;
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Successfully parsed %d actions from LLM response"),
		LastParsedConfig.Actions.Num());

	Callback.ExecuteIfBound(true, TEXT(""));
	OnParseCompleted.Broadcast(true, TEXT(""));
}

FString ULLMIntentParser::BuildPrompt(const FString& UserDescription) const
{
	return FString::Printf(TEXT("%s\n\n%s\n\nUSER: %s\nASSISTANT:"),
		*GetSystemPrompt(),
		*GetFewShotExamples(),
		*UserDescription);
}

bool ULLMIntentParser::ParseJSONResponse(const FString& JSONString, FString& OutError)
{
	// Find the JSON object in the response (LLM might add extra text)
	int32 StartIndex = JSONString.Find(TEXT("{"));
	int32 EndIndex = JSONString.Find(TEXT("}"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE || EndIndex < StartIndex)
	{
		OutError = TEXT("No valid JSON found in response");
		return false;
	}

	FString CleanJSON = JSONString.Mid(StartIndex, EndIndex - StartIndex + 1);

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CleanJSON);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		OutError = TEXT("Failed to parse JSON");
		return false;
	}

	// Clear previous config
	LastParsedConfig = FInputStreamlinerConfiguration();

	// Parse actions array
	const TArray<TSharedPtr<FJsonValue>>* ActionsArray;
	if (JsonObject->TryGetArrayField(TEXT("actions"), ActionsArray))
	{
		for (const TSharedPtr<FJsonValue>& ActionValue : *ActionsArray)
		{
			TSharedPtr<FJsonObject> ActionObj = ActionValue->AsObject();
			if (!ActionObj.IsValid()) continue;

			FInputActionDefinition ActionDef;

			ActionDef.ActionName = FName(*ActionObj->GetStringField(TEXT("name")));
			ActionDef.DisplayName = ActionObj->GetStringField(TEXT("displayName"));
			ActionDef.Category = ActionObj->GetStringField(TEXT("category"));
			ActionDef.bAllowRebinding = ActionObj->GetBoolField(TEXT("allowRebinding"));

			// Parse action type
			FString TypeStr = ActionObj->GetStringField(TEXT("type"));
			if (TypeStr == TEXT("Bool"))
				ActionDef.ActionType = EInputActionType::Bool;
			else if (TypeStr == TEXT("Axis1D"))
				ActionDef.ActionType = EInputActionType::Axis1D;
			else if (TypeStr == TEXT("Axis2D"))
				ActionDef.ActionType = EInputActionType::Axis2D;
			else if (TypeStr == TEXT("Axis3D"))
				ActionDef.ActionType = EInputActionType::Axis3D;

			// Parse bindings
			const TSharedPtr<FJsonObject>* BindingsObj;
			if (ActionObj->TryGetObjectField(TEXT("bindings"), BindingsObj))
			{
				// Parse each platform's bindings
				for (const auto& Pair : (*BindingsObj)->Values)
				{
					ETargetPlatform Platform = ETargetPlatform::None;

					if (Pair.Key == TEXT("PC_Keyboard"))
						Platform = ETargetPlatform::PC_Keyboard;
					else if (Pair.Key == TEXT("PC_Gamepad"))
						Platform = ETargetPlatform::PC_Gamepad;
					else if (Pair.Key == TEXT("Mac"))
						Platform = ETargetPlatform::Mac;
					else if (Pair.Key == TEXT("iOS"))
						Platform = ETargetPlatform::iOS;
					else if (Pair.Key == TEXT("Android"))
						Platform = ETargetPlatform::Android;

					if (Platform == ETargetPlatform::None) continue;

					FPlatformBindingConfig PlatformConfig;

					// Check if it's an array of key bindings or an object with touchControl
					const TArray<TSharedPtr<FJsonValue>>* KeysArray;
					const TSharedPtr<FJsonObject>* PlatformObj;

					if (Pair.Value->TryGetArray(KeysArray))
					{
						for (const TSharedPtr<FJsonValue>& KeyValue : *KeysArray)
						{
							TSharedPtr<FJsonObject> KeyObj = KeyValue->AsObject();
							if (!KeyObj.IsValid()) continue;

							FKeyBindingDefinition Binding;
							Binding.Key = FKey(*KeyObj->GetStringField(TEXT("key")));
							Binding.AxisMapping = KeyObj->GetStringField(TEXT("axis"));

							FString TriggerStr = KeyObj->GetStringField(TEXT("trigger"));
							if (TriggerStr == TEXT("Hold"))
								Binding.TriggerType = EInputTriggerType::Hold;

							PlatformConfig.Bindings.Add(Binding);
						}
					}
					else if (Pair.Value->TryGetObject(PlatformObj))
					{
						PlatformConfig.TouchControlType = (*PlatformObj)->GetStringField(TEXT("touchControl"));
					}

					ActionDef.PlatformBindings.Add(Platform, PlatformConfig);
				}
			}

			LastParsedConfig.Actions.Add(ActionDef);
		}
	}

	// Parse gyro config if present
	const TSharedPtr<FJsonObject>* GyroObj;
	if (JsonObject->TryGetObjectField(TEXT("gyro"), GyroObj))
	{
		LastParsedConfig.GyroConfig.bEnabled = (*GyroObj)->GetBoolField(TEXT("enabled"));
		LastParsedConfig.GyroConfig.LinkedActionName = FName(*(*GyroObj)->GetStringField(TEXT("linkedAction")));
		LastParsedConfig.GyroConfig.ActivationAction = FName(*(*GyroObj)->GetStringField(TEXT("activationAction")));
	}

	return true;
}

FString ULLMIntentParser::GetSystemPrompt()
{
	return TEXT(R"(You are an Unreal Engine 5 input configuration assistant. You parse natural language
descriptions of game input needs into structured JSON.

OUTPUT FORMAT:
{
  "actions": [
    {
      "name": "ActionName",
      "displayName": "Human Readable Name",
      "type": "Bool|Axis1D|Axis2D",
      "category": "Movement|Combat|UI|Camera",
      "allowRebinding": true,
      "bindings": {
        "PC_Keyboard": [{"key": "W"}, {"key": "A"}, {"key": "S"}, {"key": "D"}],
        "PC_Gamepad": [{"key": "Gamepad_LeftStick"}],
        "iOS": {"touchControl": "VirtualJoystick_Fixed"},
        "Android": {"touchControl": "VirtualJoystick_Fixed"}
      }
    }
  ],
  "gyro": {
    "enabled": false,
    "linkedAction": "Look",
    "activationAction": "Aim"
  }
}

KEY NAMES: Use Unreal Engine FKey names exactly:
- Keyboard: A-Z, Zero-Nine, SpaceBar, LeftShift, LeftControl, Tab, Escape
- Mouse: LeftMouseButton, RightMouseButton, MouseX, MouseY, MouseWheelUp
- Gamepad: Gamepad_LeftStick, Gamepad_RightStick, Gamepad_FaceButton_Bottom (A/Cross),
  Gamepad_FaceButton_Right (B/Circle), Gamepad_LeftTrigger, Gamepad_RightTrigger,
  Gamepad_LeftShoulder, Gamepad_RightShoulder

TOUCH CONTROLS: VirtualJoystick_Fixed, VirtualJoystick_Floating, VirtualButton,
VirtualDPad, RadialMenu, TouchRegion, GestureZone

ONLY output valid JSON. No explanations.)");
}

FString ULLMIntentParser::GetFewShotExamples()
{
	return TEXT(R"(USER: basic platformer controls
ASSISTANT: {"actions":[{"name":"Move","displayName":"Move","type":"Axis2D","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"A","axis":"-X"},{"key":"D","axis":"+X"}],"PC_Gamepad":[{"key":"Gamepad_LeftStick"}],"iOS":{"touchControl":"VirtualJoystick_Fixed"},"Android":{"touchControl":"VirtualJoystick_Fixed"}}},{"name":"Jump","displayName":"Jump","type":"Bool","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"SpaceBar"}],"PC_Gamepad":[{"key":"Gamepad_FaceButton_Bottom"}],"iOS":{"touchControl":"VirtualButton"},"Android":{"touchControl":"VirtualButton"}}}]}

USER: third person action game with dodge and lock-on
ASSISTANT: {"actions":[{"name":"Move","displayName":"Move","type":"Axis2D","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"W","axis":"+Y"},{"key":"S","axis":"-Y"},{"key":"A","axis":"-X"},{"key":"D","axis":"+X"}],"PC_Gamepad":[{"key":"Gamepad_LeftStick"}],"iOS":{"touchControl":"VirtualJoystick_Fixed"},"Android":{"touchControl":"VirtualJoystick_Fixed"}}},{"name":"Look","displayName":"Look","type":"Axis2D","category":"Camera","allowRebinding":false,"bindings":{"PC_Keyboard":[{"key":"MouseXY"}],"PC_Gamepad":[{"key":"Gamepad_RightStick"}],"iOS":{"touchControl":"TouchRegion"},"Android":{"touchControl":"TouchRegion"}}},{"name":"Dodge","displayName":"Dodge","type":"Bool","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"LeftAlt"}],"PC_Gamepad":[{"key":"Gamepad_FaceButton_Right"}],"iOS":{"touchControl":"VirtualButton"},"Android":{"touchControl":"VirtualButton"}}},{"name":"LockOn","displayName":"Lock On","type":"Bool","category":"Combat","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"Tab"}],"PC_Gamepad":[{"key":"Gamepad_RightShoulder"}],"iOS":{"touchControl":"VirtualButton"},"Android":{"touchControl":"VirtualButton"}}}]})");
}
