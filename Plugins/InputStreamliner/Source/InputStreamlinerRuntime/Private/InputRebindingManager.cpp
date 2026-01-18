// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputRebindingManager.h"
#include "InputStreamlinerRuntimeModule.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/InputSettings.h"

void UInputRebindingManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("InputRebindingManager initialized"));

	// Load saved bindings
	LoadBindings();
}

void UInputRebindingManager::Deinitialize()
{
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

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Started rebinding for action: %s"), *Action->GetName());

	// In a real implementation, we would hook into the input system here
	// to capture the next key press. For now, this is a placeholder.
}

void UInputRebindingManager::CancelRebinding()
{
	if (PendingRebindAction)
	{
		UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Cancelled rebinding for action: %s"),
			*PendingRebindAction->GetName());
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

	// Get or create binding array
	TArray<FKey>& Bindings = CurrentBindings.FindOrAdd(Action);

	// Ensure array is large enough
	while (Bindings.Num() <= BindingIndex)
	{
		Bindings.Add(EKeys::Invalid);
	}

	Bindings[BindingIndex] = NewKey;

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Applied binding %s to action %s at index %d"),
		*NewKey.ToString(), *Action->GetName(), BindingIndex);

	OnRebindComplete.Broadcast(Action, NewKey);

	// Clear rebinding state
	if (PendingRebindAction == Action)
	{
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

	(*Bindings)[BindingIndex] = EKeys::Invalid;
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
	}

	// Add to A
	if (IndexA == INDEX_NONE)
	{
		BindingsA.Add(Key);
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

	const TArray<FKey>* Defaults = DefaultBindings.Find(Action);
	if (Defaults)
	{
		CurrentBindings.Add(Action, *Defaults);
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

	// Save to slot
	// Note: In a real implementation, you would use USaveGame
	// For now, we'll use a simple JSON file approach

	FString SavePath = FPaths::ProjectSavedDir() / TEXT("InputStreamliner") / TEXT("Bindings.json");

	FString JsonString;
	// TODO: Serialize SaveData to JSON and write to file

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Bindings saved"));
	return true;
}

bool UInputRebindingManager::LoadBindings()
{
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("InputStreamliner") / TEXT("Bindings.json");

	if (!FPaths::FileExists(SavePath))
	{
		UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("No saved bindings found"));
		return false;
	}

	// TODO: Load and deserialize JSON

	ApplyLoadedBindings();

	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("Bindings loaded"));
	return true;
}

void UInputRebindingManager::RegisterAction(UInputAction* Action, const TArray<FKey>& Bindings)
{
	if (!Action)
	{
		return;
	}

	DefaultBindings.Add(Action, Bindings);

	// If no custom binding exists, use default
	if (!CurrentBindings.Contains(Action))
	{
		CurrentBindings.Add(Action, Bindings);
	}

	UE_LOG(LogInputStreamlinerRuntime, Verbose, TEXT("Registered action %s with %d default bindings"),
		*Action->GetName(), Bindings.Num());
}

void UInputRebindingManager::HandleKeyDown(const FKeyEvent& KeyEvent)
{
	FKey Key = KeyEvent.GetKey();
	OnAnyKeyPressed.Broadcast(Key);

	if (PendingRebindAction)
	{
		// Ignore certain keys
		if (Key == EKeys::Escape)
		{
			CancelRebinding();
			return;
		}

		ApplyBinding(PendingRebindAction, Key, PendingBindingIndex);
	}
}

void UInputRebindingManager::HandleGamepadInput(const FKeyEvent& KeyEvent)
{
	// Same handling as keyboard for now
	HandleKeyDown(KeyEvent);
}

void UInputRebindingManager::ApplyLoadedBindings()
{
	// TODO: Apply bindings to the actual Enhanced Input Mapping Contexts
	// This requires runtime modification of the mapping contexts
}
