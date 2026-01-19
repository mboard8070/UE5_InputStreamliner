// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputRebindingManager.h"
#include "InputStreamlinerRuntimeModule.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/InputSettings.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"
#include "Framework/Application/SlateApplication.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"

void UInputRebindingManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create input processor for capturing rebind keys
	RebindInputProcessor = MakeShareable(new FRebindInputProcessor(this));

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("InputRebindingManager initialized"));

	// Load saved bindings
	LoadBindings();
}

void UInputRebindingManager::Deinitialize()
{
	// Remove input processor
	if (RebindInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(RebindInputProcessor);
	}
	RebindInputProcessor.Reset();

	// Auto-save on shutdown
	SaveBindings();

	Super::Deinitialize();
}

void UInputRebindingManager::StartRebinding(UInputAction* Action)
{
	if (!Action)
	{
		UE_LOG(LogInputStreamlinerRuntime, Warning, TEXT("Cannot start rebinding: null action"));
		return;
	}

	PendingRebindAction = Action;
	PendingBindingIndex = 0;

	// Register input processor to capture next key press
	if (RebindInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().RegisterInputPreProcessor(RebindInputProcessor);
	}

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Started rebinding for action: %s"), *Action->GetName());
}

void UInputRebindingManager::CancelRebinding()
{
	if (PendingRebindAction)
	{
		UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Cancelled rebinding for action: %s"),
			*PendingRebindAction->GetName());
	}

	// Unregister input processor
	if (RebindInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(RebindInputProcessor);
	}

	PendingRebindAction = nullptr;
	PendingBindingIndex = 0;
}

TArray<FKey> UInputRebindingManager::GetBindingsForAction(UInputAction* Action) const
{
	if (!Action)
	{
		return TArray<FKey>();
	}

	const TArray<FKey>* CustomBindings = CurrentBindings.Find(Action);
	if (CustomBindings)
	{
		return *CustomBindings;
	}

	const TArray<FKey>* Defaults = DefaultBindings.Find(Action);
	if (Defaults)
	{
		return *Defaults;
	}

	return TArray<FKey>();
}

bool UInputRebindingManager::ApplyBinding(UInputAction* Action, FKey NewKey, int32 BindingIndex)
{
	if (!Action || !NewKey.IsValid())
	{
		return false;
	}

	// Check for conflicts
	UInputAction* ConflictingAction = nullptr;
	if (HasConflict(Action, NewKey, ConflictingAction))
	{
		OnBindingConflict.Broadcast(ConflictingAction, NewKey);
		// Don't apply - let the UI handle conflict resolution
		return false;
	}

	// Get old key for mapping context update
	TArray<FKey> OldBindings = GetBindingsForAction(Action);
	FKey OldKey = OldBindings.IsValidIndex(BindingIndex) ? OldBindings[BindingIndex] : EKeys::Invalid;

	// Get or create binding array
	TArray<FKey>& Bindings = CurrentBindings.FindOrAdd(Action);

	// Ensure array is large enough
	while (Bindings.Num() <= BindingIndex)
	{
		Bindings.Add(EKeys::Invalid);
	}

	Bindings[BindingIndex] = NewKey;

	// Apply to mapping context
	ApplyBindingToMappingContext(Action, OldKey, NewKey);

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Applied binding %s to action %s at index %d"),
		*NewKey.ToString(), *Action->GetName(), BindingIndex);

	OnRebindComplete.Broadcast(Action, NewKey);

	// Clear rebinding state
	if (PendingRebindAction == Action)
	{
		// Unregister input processor
		if (RebindInputProcessor.IsValid() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(RebindInputProcessor);
		}
		PendingRebindAction = nullptr;
	}

	return true;
}

bool UInputRebindingManager::RemoveBinding(UInputAction* Action, int32 BindingIndex)
{
	if (!Action)
	{
		return false;
	}

	TArray<FKey>* Bindings = CurrentBindings.Find(Action);
	if (!Bindings || !Bindings->IsValidIndex(BindingIndex))
	{
		return false;
	}

	FKey OldKey = (*Bindings)[BindingIndex];
	(*Bindings)[BindingIndex] = EKeys::Invalid;

	// Update mapping context
	ApplyBindingToMappingContext(Action, OldKey, EKeys::Invalid);

	return true;
}

bool UInputRebindingManager::HasConflict(UInputAction* Action, FKey Key, UInputAction*& OutConflictingAction) const
{
	OutConflictingAction = nullptr;

	for (const auto& Pair : CurrentBindings)
	{
		if (Pair.Key == Action)
		{
			continue; // Skip self
		}

		if (Pair.Value.Contains(Key))
		{
			OutConflictingAction = Pair.Key;
			return true;
		}
	}

	return false;
}

void UInputRebindingManager::SwapBindings(UInputAction* ActionA, UInputAction* ActionB, FKey Key)
{
	if (!ActionA || !ActionB)
	{
		return;
	}

	TArray<FKey>& BindingsA = CurrentBindings.FindOrAdd(ActionA);
	TArray<FKey>& BindingsB = CurrentBindings.FindOrAdd(ActionB);

	// Find indices
	int32 IndexA = BindingsA.Find(Key);
	int32 IndexB = BindingsB.Find(Key);

	if (IndexB != INDEX_NONE)
	{
		// Remove from B
		BindingsB[IndexB] = EKeys::Invalid;
		ApplyBindingToMappingContext(ActionB, Key, EKeys::Invalid);
	}

	// Add to A
	if (IndexA == INDEX_NONE)
	{
		BindingsA.Add(Key);
		ApplyBindingToMappingContext(ActionA, EKeys::Invalid, Key);
	}

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Swapped binding %s from %s to %s"),
		*Key.ToString(), *ActionB->GetName(), *ActionA->GetName());
}

void UInputRebindingManager::ResetToDefault(UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	// Get current bindings to remove from mapping context
	TArray<FKey> CurrentKeys = GetBindingsForAction(Action);

	const TArray<FKey>* Defaults = DefaultBindings.Find(Action);
	if (Defaults)
	{
		CurrentBindings.Add(Action, *Defaults);

		// Update mapping context - remove old, add defaults
		for (const FKey& OldKey : CurrentKeys)
		{
			if (OldKey.IsValid() && !Defaults->Contains(OldKey))
			{
				ApplyBindingToMappingContext(Action, OldKey, EKeys::Invalid);
			}
		}
		for (const FKey& NewKey : *Defaults)
		{
			if (NewKey.IsValid() && !CurrentKeys.Contains(NewKey))
			{
				ApplyBindingToMappingContext(Action, EKeys::Invalid, NewKey);
			}
		}
	}
	else
	{
		CurrentBindings.Remove(Action);
	}

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Reset action %s to defaults"), *Action->GetName());
}

void UInputRebindingManager::ResetAllToDefaults()
{
	CurrentBindings.Empty();

	for (const auto& Pair : DefaultBindings)
	{
		CurrentBindings.Add(Pair.Key, Pair.Value);
	}

	// Reset sensitivity settings
	SaveData.MouseSensitivity = 1.0f;
	SaveData.GamepadSensitivity = 1.0f;
	SaveData.GyroSensitivity = 1.0f;
	SaveData.bInvertY = false;

	// Reapply all bindings to mapping context
	ApplyLoadedBindings();

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Reset all bindings to defaults"));
}

void UInputRebindingManager::SetMouseSensitivity(float Sensitivity)
{
	SaveData.MouseSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
}

void UInputRebindingManager::SetGamepadSensitivity(float Sensitivity)
{
	SaveData.GamepadSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
}

void UInputRebindingManager::SetGyroSensitivity(float Sensitivity)
{
	SaveData.GyroSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
}

void UInputRebindingManager::SetInvertY(bool bInvert)
{
	SaveData.bInvertY = bInvert;
}

bool UInputRebindingManager::SaveBindings()
{
	// Convert current bindings to save format
	SaveData.Bindings.Empty();

	for (const auto& Pair : CurrentBindings)
	{
		if (!Pair.Key)
		{
			continue;
		}

		FActionBindingSave BindingSave;
		BindingSave.ActionName = Pair.Key->GetFName();
		BindingSave.Keys = Pair.Value;
		SaveData.Bindings.Add(BindingSave);
	}

	// Serialize to JSON
	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(SaveData, JsonString))
	{
		UE_LOG(LogInputStreamlinerRuntime, Error, TEXT("Failed to serialize bindings to JSON"));
		return false;
	}

	// Ensure directory exists
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("InputStreamliner");
	IFileManager::Get().MakeDirectory(*SaveDir, true);

	// Write to file
	FString SavePath = SaveDir / TEXT("Bindings.json");
	if (!FFileHelper::SaveStringToFile(JsonString, *SavePath))
	{
		UE_LOG(LogInputStreamlinerRuntime, Error, TEXT("Failed to save bindings to: %s"), *SavePath);
		return false;
	}

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Bindings saved to: %s"), *SavePath);
	return true;
}

bool UInputRebindingManager::LoadBindings()
{
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("InputStreamliner") / TEXT("Bindings.json");

	if (!FPaths::FileExists(SavePath))
	{
		UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("No saved bindings found at: %s"), *SavePath);
		return false;
	}

	// Read file
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *SavePath))
	{
		UE_LOG(LogInputStreamlinerRuntime, Error, TEXT("Failed to load bindings from: %s"), *SavePath);
		return false;
	}

	// Deserialize JSON
	FInputBindingSaveData LoadedData;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &LoadedData))
	{
		UE_LOG(LogInputStreamlinerRuntime, Error, TEXT("Failed to parse bindings JSON"));
		return false;
	}

	SaveData = LoadedData;

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Bindings loaded from: %s"), *SavePath);
	return true;
}

void UInputRebindingManager::RegisterAction(UInputAction* Action, const TArray<FKey>& Bindings)
{
	if (!Action)
	{
		return;
	}

	DefaultBindings.Add(Action, Bindings);

	// Check if we have saved bindings for this action
	bool bFoundSaved = false;
	for (const FActionBindingSave& Saved : SaveData.Bindings)
	{
		if (Saved.ActionName == Action->GetFName())
		{
			CurrentBindings.Add(Action, Saved.Keys);
			bFoundSaved = true;
			break;
		}
	}

	// If no saved binding exists, use default
	if (!bFoundSaved)
	{
		CurrentBindings.Add(Action, Bindings);
	}

	UE_LOG(LogInputStreamlinerRuntime, Verbose, TEXT("Registered action %s with %d default bindings"),
		*Action->GetName(), Bindings.Num());
}

void UInputRebindingManager::SetMappingContext(UInputMappingContext* Context)
{
	ActiveMappingContext = Context;

	if (Context)
	{
		// Apply any loaded custom bindings
		ApplyLoadedBindings();
	}
}

bool UInputRebindingManager::HandleKeyDown(const FKeyEvent& KeyEvent)
{
	if (!PendingRebindAction)
	{
		return false;
	}

	FKey Key = KeyEvent.GetKey();
	OnAnyKeyPressed.Broadcast(Key);

	// Ignore certain keys
	if (Key == EKeys::Escape)
	{
		CancelRebinding();
		return true;
	}

	// Ignore modifier keys by themselves
	if (Key == EKeys::LeftShift || Key == EKeys::RightShift ||
		Key == EKeys::LeftControl || Key == EKeys::RightControl ||
		Key == EKeys::LeftAlt || Key == EKeys::RightAlt ||
		Key == EKeys::LeftCommand || Key == EKeys::RightCommand)
	{
		return true; // Consume but don't apply
	}

	ApplyBinding(PendingRebindAction, Key, PendingBindingIndex);
	return true; // Consume the input
}

bool UInputRebindingManager::HandleAnalogInput(const FAnalogInputEvent& AnalogEvent)
{
	if (!PendingRebindAction)
	{
		return false;
	}

	// Only capture significant analog input
	if (FMath::Abs(AnalogEvent.GetAnalogValue()) < 0.5f)
	{
		return false;
	}

	FKey Key = AnalogEvent.GetKey();
	OnAnyKeyPressed.Broadcast(Key);

	ApplyBinding(PendingRebindAction, Key, PendingBindingIndex);
	return true;
}

void UInputRebindingManager::ApplyLoadedBindings()
{
	if (!ActiveMappingContext)
	{
		return;
	}

	// Get the Enhanced Input subsystem
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();
	if (!Subsystem)
	{
		return;
	}

	// For each registered action, update the mapping context
	for (const auto& Pair : CurrentBindings)
	{
		UInputAction* Action = Pair.Key;
		const TArray<FKey>& CustomKeys = Pair.Value;
		const TArray<FKey>* DefaultKeys = DefaultBindings.Find(Action);

		if (!Action || !DefaultKeys)
		{
			continue;
		}

		// Check if bindings differ from defaults
		for (int32 i = 0; i < CustomKeys.Num(); i++)
		{
			FKey CustomKey = CustomKeys[i];
			FKey DefaultKey = DefaultKeys->IsValidIndex(i) ? (*DefaultKeys)[i] : EKeys::Invalid;

			if (CustomKey != DefaultKey)
			{
				ApplyBindingToMappingContext(Action, DefaultKey, CustomKey);
			}
		}
	}

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Applied loaded bindings to mapping context"));
}

void UInputRebindingManager::ApplyBindingToMappingContext(UInputAction* Action, FKey OldKey, FKey NewKey)
{
	if (!ActiveMappingContext || !Action)
	{
		return;
	}

	// Get mutable mappings
	TArray<FEnhancedActionKeyMapping>& Mappings = const_cast<TArray<FEnhancedActionKeyMapping>&>(ActiveMappingContext->GetMappings());

	// Find and update the mapping
	bool bFoundMapping = false;
	for (FEnhancedActionKeyMapping& Mapping : Mappings)
	{
		if (Mapping.Action == Action && Mapping.Key == OldKey)
		{
			if (NewKey.IsValid())
			{
				Mapping.Key = NewKey;
				UE_LOG(LogInputStreamlinerRuntime, Verbose, TEXT("Updated mapping: %s -> %s for action %s"),
					*OldKey.ToString(), *NewKey.ToString(), *Action->GetName());
			}
			else
			{
				// Remove the mapping by setting to invalid (actual removal is complex)
				Mapping.Key = EKeys::Invalid;
			}
			bFoundMapping = true;
			break;
		}
	}

	// If adding a new binding and no existing mapping found
	if (!bFoundMapping && NewKey.IsValid() && !OldKey.IsValid())
	{
		// Add new mapping
		FEnhancedActionKeyMapping NewMapping;
		NewMapping.Action = Action;
		NewMapping.Key = NewKey;
		Mappings.Add(NewMapping);
		UE_LOG(LogInputStreamlinerRuntime, Verbose, TEXT("Added new mapping: %s for action %s"),
			*NewKey.ToString(), *Action->GetName());
	}

	// Request rebuild of the input system
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();
	if (Subsystem)
	{
		// Force rebind by removing and re-adding the context
		int32 Priority = 0;
		Subsystem->RemoveMappingContext(ActiveMappingContext);
		Subsystem->AddMappingContext(ActiveMappingContext, Priority);
	}
}

UEnhancedInputLocalPlayerSubsystem* UInputRebindingManager::GetEnhancedInputSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	APlayerController* PC = GameInstance->GetFirstLocalPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

// ============== FRebindInputProcessor Implementation ==============

bool FRebindInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (Manager.IsValid() && Manager->IsRebindingInProgress())
	{
		return Manager->HandleKeyDown(InKeyEvent);
	}
	return false;
}

bool FRebindInputProcessor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	if (Manager.IsValid() && Manager->IsRebindingInProgress())
	{
		return Manager->HandleAnalogInput(InAnalogInputEvent);
	}
	return false;
}

bool FRebindInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (Manager.IsValid() && Manager->IsRebindingInProgress())
	{
		FKey Key = MouseEvent.GetEffectingButton();
		Manager->OnAnyKeyPressed.Broadcast(Key);

		// Don't bind left mouse click (used for UI interaction)
		if (Key == EKeys::LeftMouseButton)
		{
			return false;
		}

		Manager->ApplyBinding(Manager->GetPendingRebindAction(), Key, 0);
		return true;
	}
	return false;
}

bool FRebindInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent)
{
	if (Manager.IsValid() && Manager->IsRebindingInProgress())
	{
		FKey Key = InWheelEvent.GetWheelDelta() > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown;
		Manager->OnAnyKeyPressed.Broadcast(Key);
		Manager->ApplyBinding(Manager->GetPendingRebindAction(), Key, 0);
		return true;
	}
	return false;
}
