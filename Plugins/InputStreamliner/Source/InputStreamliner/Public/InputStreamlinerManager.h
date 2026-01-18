// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "InputStreamlinerConfiguration.h"
#include "InputStreamlinerManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionAdded, FName, ActionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionRemoved, FName, ActionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionUpdated, FName, ActionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfigurationChanged);

/**
 * Editor subsystem that manages the Input Streamliner configuration
 * Handles dynamic add/remove/update of input actions
 */
UCLASS(BlueprintType)
class INPUTSTREAMLINER_API UInputStreamlinerManager : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin UEditorSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UEditorSubsystem Interface

	// Action Management

	/** Add a new input action to the configuration */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	bool AddInputAction(const FInputActionDefinition& NewAction);

	/** Remove an input action by name */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	bool RemoveInputAction(FName ActionName);

	/** Update an existing input action */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	bool UpdateInputAction(FName ActionName, const FInputActionDefinition& UpdatedAction);

	/** Duplicate an existing action as a starting point */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	bool DuplicateInputAction(FName SourceActionName, FName NewActionName);

	/** Reorder an action in the list */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	void ReorderAction(FName ActionName, int32 NewIndex);

	// Bulk Operations

	/** Remove all input actions */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	void RemoveAllActions();

	/** Import actions from an existing Input Mapping Context */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
	void ImportActionsFromContext(class UInputMappingContext* ExistingContext);

	// Touch Control Management

	/** Add a new touch control */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	bool AddTouchControl(const FTouchControlDefinition& NewControl);

	/** Remove a touch control by name */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	bool RemoveTouchControl(FName ControlName);

	/** Update an existing touch control */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	bool UpdateTouchControl(FName ControlName, const FTouchControlDefinition& UpdatedControl);

	// Accessors

	/** Get the current configuration */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
	const FInputStreamlinerConfiguration& GetCurrentConfiguration() const { return CurrentConfig; }

	/** Get a mutable reference to the configuration (use with care) */
	FInputStreamlinerConfiguration& GetMutableConfiguration() { return CurrentConfig; }

	/** Get an action by name */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
	bool GetActionByName(FName ActionName, FInputActionDefinition& OutAction) const;

	/** Get all action names */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
	TArray<FName> GetAllActionNames() const;

	// Validation

	/** Check if an action name is unique */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Validation")
	bool IsActionNameUnique(FName ActionName) const;

	/** Validate an action definition */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Validation")
	bool ValidateAction(const FInputActionDefinition& Action, FString& OutError) const;

	// Persistence

	/** Save configuration to disk */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Persistence")
	bool SaveConfiguration();

	/** Load configuration from disk */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Persistence")
	bool LoadConfiguration();

	/** Get the path to the configuration file */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Persistence")
	FString GetConfigurationFilePath() const;

	// Events

	/** Called when an action is added */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnActionAdded OnActionAdded;

	/** Called when an action is removed */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnActionRemoved OnActionRemoved;

	/** Called when an action is updated */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnActionUpdated OnActionUpdated;

	/** Called when the configuration changes */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnConfigurationChanged OnConfigurationChanged;

private:
	/** Current configuration */
	UPROPERTY()
	FInputStreamlinerConfiguration CurrentConfig;

	/** Notify listeners that the configuration has changed */
	void NotifyConfigurationChanged();

	/** Generate a unique action name based on a base name */
	FName GenerateUniqueActionName(const FString& BaseName) const;
};
