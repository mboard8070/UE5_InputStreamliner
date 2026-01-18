// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputStreamlinerManager.h"
#include "InputStreamlinerModule.h"
#include "InputMappingContext.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"
#include "HAL/PlatformFileManager.h"

void UInputStreamlinerManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogInputStreamliner, Log, TEXT("InputStreamlinerManager initialized"));

	// Try to load existing configuration
	LoadConfiguration();
}

void UInputStreamlinerManager::Deinitialize()
{
	// Auto-save configuration on shutdown
	SaveConfiguration();

	Super::Deinitialize();
}

bool UInputStreamlinerManager::AddInputAction(const FInputActionDefinition& NewAction)
{
	// Validate the action
	FString ValidationError;
	if (!ValidateAction(NewAction, ValidationError))
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Failed to add action: %s"), *ValidationError);
		return false;
	}

	// Check for duplicate names
	if (!IsActionNameUnique(NewAction.ActionName))
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Action name '%s' already exists"), *NewAction.ActionName.ToString());
		return false;
	}

	// Add to configuration
	CurrentConfig.Actions.Add(NewAction);

	// Broadcast events
	OnActionAdded.Broadcast(NewAction.ActionName);
	NotifyConfigurationChanged();

	UE_LOG(LogInputStreamliner, Log, TEXT("Added input action: %s"), *NewAction.ActionName.ToString());
	return true;
}

bool UInputStreamlinerManager::RemoveInputAction(FName ActionName)
{
	int32 IndexToRemove = CurrentConfig.Actions.IndexOfByPredicate(
		[ActionName](const FInputActionDefinition& Action)
		{
			return Action.ActionName == ActionName;
		});

	if (IndexToRemove == INDEX_NONE)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Action '%s' not found for removal"), *ActionName.ToString());
		return false;
	}

	// Remove associated touch controls
	CurrentConfig.TouchControls.RemoveAll(
		[ActionName](const FTouchControlDefinition& Control)
		{
			return Control.LinkedActionName == ActionName;
		});

	// Remove the action
	CurrentConfig.Actions.RemoveAt(IndexToRemove);

	// Broadcast events
	OnActionRemoved.Broadcast(ActionName);
	NotifyConfigurationChanged();

	UE_LOG(LogInputStreamliner, Log, TEXT("Removed input action: %s"), *ActionName.ToString());
	return true;
}

bool UInputStreamlinerManager::UpdateInputAction(FName ActionName, const FInputActionDefinition& UpdatedAction)
{
	FInputActionDefinition* ExistingAction = CurrentConfig.FindAction(ActionName);

	if (!ExistingAction)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Action '%s' not found for update"), *ActionName.ToString());
		return false;
	}

	// If name changed, check for conflicts
	if (UpdatedAction.ActionName != ActionName)
	{
		if (!IsActionNameUnique(UpdatedAction.ActionName))
		{
			UE_LOG(LogInputStreamliner, Warning, TEXT("New action name '%s' already exists"),
				*UpdatedAction.ActionName.ToString());
			return false;
		}

		// Update touch controls if name changed
		for (FTouchControlDefinition& Control : CurrentConfig.TouchControls)
		{
			if (Control.LinkedActionName == ActionName)
			{
				Control.LinkedActionName = UpdatedAction.ActionName;
			}
		}
	}

	// Apply update
	*ExistingAction = UpdatedAction;

	// Broadcast events
	OnActionUpdated.Broadcast(UpdatedAction.ActionName);
	NotifyConfigurationChanged();

	UE_LOG(LogInputStreamliner, Log, TEXT("Updated input action: %s"), *UpdatedAction.ActionName.ToString());
	return true;
}

bool UInputStreamlinerManager::DuplicateInputAction(FName SourceActionName, FName NewActionName)
{
	const FInputActionDefinition* SourceAction = CurrentConfig.FindAction(SourceActionName);

	if (!SourceAction)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Source action '%s' not found for duplication"),
			*SourceActionName.ToString());
		return false;
	}

	// Generate unique name if requested name conflicts
	FName FinalName = NewActionName;
	if (!IsActionNameUnique(FinalName))
	{
		FinalName = GenerateUniqueActionName(NewActionName.ToString());
	}

	// Create duplicate
	FInputActionDefinition NewAction = *SourceAction;
	NewAction.ActionName = FinalName;
	NewAction.DisplayName = FString::Printf(TEXT("%s (Copy)"), *SourceAction->DisplayName);

	return AddInputAction(NewAction);
}

void UInputStreamlinerManager::ReorderAction(FName ActionName, int32 NewIndex)
{
	int32 CurrentIndex = CurrentConfig.Actions.IndexOfByPredicate(
		[ActionName](const FInputActionDefinition& Action)
		{
			return Action.ActionName == ActionName;
		});

	if (CurrentIndex == INDEX_NONE)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Action '%s' not found for reorder"), *ActionName.ToString());
		return;
	}

	// Clamp new index to valid range
	NewIndex = FMath::Clamp(NewIndex, 0, CurrentConfig.Actions.Num() - 1);

	if (CurrentIndex == NewIndex)
	{
		return; // No change needed
	}

	// Remove and reinsert at new position
	FInputActionDefinition Action = CurrentConfig.Actions[CurrentIndex];
	CurrentConfig.Actions.RemoveAt(CurrentIndex);
	CurrentConfig.Actions.Insert(Action, NewIndex);

	NotifyConfigurationChanged();
}

void UInputStreamlinerManager::RemoveAllActions()
{
	CurrentConfig.Actions.Empty();
	CurrentConfig.TouchControls.Empty();

	NotifyConfigurationChanged();

	UE_LOG(LogInputStreamliner, Log, TEXT("Removed all input actions"));
}

void UInputStreamlinerManager::ImportActionsFromContext(UInputMappingContext* ExistingContext)
{
	if (!ExistingContext)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Cannot import from null mapping context"));
		return;
	}

	// TODO: Implement import logic from existing UInputMappingContext
	// This will iterate over the context's mappings and create FInputActionDefinitions

	UE_LOG(LogInputStreamliner, Log, TEXT("Import from context not yet implemented"));
}

bool UInputStreamlinerManager::AddTouchControl(const FTouchControlDefinition& NewControl)
{
	// Check for duplicate names
	bool bExists = CurrentConfig.TouchControls.ContainsByPredicate(
		[&NewControl](const FTouchControlDefinition& Control)
		{
			return Control.ControlName == NewControl.ControlName;
		});

	if (bExists)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Touch control '%s' already exists"),
			*NewControl.ControlName.ToString());
		return false;
	}

	CurrentConfig.TouchControls.Add(NewControl);
	NotifyConfigurationChanged();

	return true;
}

bool UInputStreamlinerManager::RemoveTouchControl(FName ControlName)
{
	int32 Removed = CurrentConfig.TouchControls.RemoveAll(
		[ControlName](const FTouchControlDefinition& Control)
		{
			return Control.ControlName == ControlName;
		});

	if (Removed > 0)
	{
		NotifyConfigurationChanged();
		return true;
	}

	return false;
}

bool UInputStreamlinerManager::UpdateTouchControl(FName ControlName, const FTouchControlDefinition& UpdatedControl)
{
	FTouchControlDefinition* ExistingControl = CurrentConfig.TouchControls.FindByPredicate(
		[ControlName](const FTouchControlDefinition& Control)
		{
			return Control.ControlName == ControlName;
		});

	if (!ExistingControl)
	{
		return false;
	}

	*ExistingControl = UpdatedControl;
	NotifyConfigurationChanged();

	return true;
}

bool UInputStreamlinerManager::GetActionByName(FName ActionName, FInputActionDefinition& OutAction) const
{
	const FInputActionDefinition* Found = CurrentConfig.FindAction(ActionName);
	if (Found)
	{
		OutAction = *Found;
		return true;
	}
	return false;
}

TArray<FName> UInputStreamlinerManager::GetAllActionNames() const
{
	return CurrentConfig.GetActionNames();
}

bool UInputStreamlinerManager::IsActionNameUnique(FName ActionName) const
{
	return !CurrentConfig.HasAction(ActionName);
}

bool UInputStreamlinerManager::ValidateAction(const FInputActionDefinition& Action, FString& OutError) const
{
	if (Action.ActionName.IsNone())
	{
		OutError = TEXT("Action name cannot be empty");
		return false;
	}

	if (Action.DisplayName.IsEmpty())
	{
		OutError = TEXT("Display name cannot be empty");
		return false;
	}

	if (Action.TargetPlatforms == 0)
	{
		OutError = TEXT("At least one target platform must be selected");
		return false;
	}

	return true;
}

bool UInputStreamlinerManager::SaveConfiguration()
{
	FString ConfigPath = GetConfigurationFilePath();

	// Ensure directory exists
	FString Directory = FPaths::GetPath(ConfigPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	// Convert to JSON
	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(CurrentConfig, JsonString))
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to serialize configuration to JSON"));
		return false;
	}

	// Write to file
	if (!FFileHelper::SaveStringToFile(JsonString, *ConfigPath))
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to save configuration to: %s"), *ConfigPath);
		return false;
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Configuration saved to: %s"), *ConfigPath);
	return true;
}

bool UInputStreamlinerManager::LoadConfiguration()
{
	FString ConfigPath = GetConfigurationFilePath();

	if (!FPaths::FileExists(ConfigPath))
	{
		UE_LOG(LogInputStreamliner, Log, TEXT("No existing configuration found at: %s"), *ConfigPath);
		return false;
	}

	// Read file
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to load configuration from: %s"), *ConfigPath);
		return false;
	}

	// Parse JSON
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &CurrentConfig))
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to parse configuration JSON"));
		return false;
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Configuration loaded from: %s"), *ConfigPath);
	return true;
}

FString UInputStreamlinerManager::GetConfigurationFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("InputStreamliner") / TEXT("Configuration.json");
}

void UInputStreamlinerManager::NotifyConfigurationChanged()
{
	OnConfigurationChanged.Broadcast();
}

FName UInputStreamlinerManager::GenerateUniqueActionName(const FString& BaseName) const
{
	FString TestName = BaseName;
	int32 Counter = 1;

	while (!IsActionNameUnique(FName(*TestName)))
	{
		TestName = FString::Printf(TEXT("%s_%d"), *BaseName, Counter++);
	}

	return FName(*TestName);
}
