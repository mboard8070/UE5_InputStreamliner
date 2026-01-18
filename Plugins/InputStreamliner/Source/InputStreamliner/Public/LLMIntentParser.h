// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputStreamlinerConfiguration.h"
#include "Interfaces/IHttpRequest.h"
#include "LLMIntentParser.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnParseComplete, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnParseCompleteMulticast, bool, bSuccess, const FString&, ErrorMessage);

/**
 * Handles parsing natural language input descriptions using a local LLM (Ollama)
 */
UCLASS(BlueprintType)
class INPUTSTREAMLINER_API ULLMIntentParser : public UObject
{
	GENERATED_BODY()

public:
	ULLMIntentParser();

	// Configuration

	/** Set the LLM endpoint URL */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void SetEndpoint(const FString& URL, int32 Port = 11434);

	/** Set the model to use */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void SetModel(const FString& ModelName);

	/** Check if the LLM endpoint is reachable */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void CheckConnection(FOnParseComplete OnComplete);

	// Parsing

	/** Parse a natural language description asynchronously */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void ParseInputDescriptionAsync(const FString& Description, FOnParseComplete OnComplete);

	/** Get the last parsed configuration */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|LLM")
	const FInputStreamlinerConfiguration& GetLastParsedConfiguration() const { return LastParsedConfig; }

	/** Check if parsing is currently in progress */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|LLM")
	bool IsParseInProgress() const { return bParseInProgress; }

	// Configuration accessors

	UFUNCTION(BlueprintPure, Category = "Input Streamliner|LLM")
	FString GetEndpointURL() const { return EndpointURL; }

	UFUNCTION(BlueprintPure, Category = "Input Streamliner|LLM")
	int32 GetEndpointPort() const { return EndpointPort; }

	UFUNCTION(BlueprintPure, Category = "Input Streamliner|LLM")
	FString GetModelName() const { return ModelName; }

	// Events

	/** Called when parsing completes */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|LLM")
	FOnParseCompleteMulticast OnParseCompleted;

private:
	/** Build the complete prompt including system prompt and examples */
	FString BuildPrompt(const FString& UserDescription) const;

	/** Parse the JSON response from the LLM */
	bool ParseJSONResponse(const FString& JSONString, FString& OutError);

	/** Handle HTTP response */
	void OnHttpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnParseComplete Callback);

	/** Get the system prompt for the LLM */
	static FString GetSystemPrompt();

	/** Get few-shot examples */
	static FString GetFewShotExamples();

	UPROPERTY()
	FString EndpointURL = TEXT("http://localhost");

	UPROPERTY()
	int32 EndpointPort = 11434;

	UPROPERTY()
	FString ModelName = TEXT("nemotron:8b");

	UPROPERTY()
	FInputStreamlinerConfiguration LastParsedConfig;

	bool bParseInProgress = false;
};
