// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "InputStreamlinerConfiguration.h"
#include "InputActionDefinition.h"
#include "TouchControlDefinition.h"
#include "Interfaces/IHttpRequest.h"
#include "InputStreamlinerWidget.generated.h"

class ULLMIntentParser;
class UInputStreamlinerManager;
class UInputAssetGenerator;
class UVerticalBox;
class UHorizontalBox;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UScrollBox;
class UEditableTextBox;
class UComboBoxString;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConfigurationUpdated, const FInputStreamlinerConfiguration&, Configuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLLMParseComplete, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGenerationComplete, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionSelected, const FInputActionDefinition&, Action);

/**
 * Base class for the Input Streamliner Editor Widget
 * Provides core functionality for the natural language input configuration tool
 */
UCLASS(BlueprintType, Blueprintable, meta=(ShowWorldContextPin))
class INPUTSTREAMLINER_API UInputStreamlinerWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UInputStreamlinerWidget();

	//~ Begin UUserWidget Interface
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	// ==================== LLM Integration ====================

	/** Parse natural language description using LLM */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void ParseDescription(const FString& Description);

	/** Check if LLM is currently parsing */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|LLM")
	bool IsParsingInProgress() const;

	/** Test connection to the LLM endpoint */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void TestLLMConnection();

	/** Set LLM endpoint configuration */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
	void SetLLMEndpoint(const FString& URL, int32 Port, const FString& ModelName);

	// ==================== Configuration Management ====================

	/** Get the current configuration */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Configuration")
	const FInputStreamlinerConfiguration& GetConfiguration() const { return CurrentConfiguration; }

	/** Set the entire configuration */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Configuration")
	void SetConfiguration(const FInputStreamlinerConfiguration& NewConfiguration);

	/** Set the project prefix */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Configuration")
	void SetProjectPrefix(const FString& Prefix);

	/** Set code generation type */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Configuration")
	void SetCodeGenerationType(ECodeGenerationType Type);

	/** Set output paths */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Configuration")
	void SetOutputPaths(const FString& ActionsPath, const FString& ContextsPath, const FString& WidgetsPath, const FString& CodePath);

	// ==================== Action Management ====================

	/** Get all actions */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Actions")
	TArray<FInputActionDefinition> GetAllActions() const { return CurrentConfiguration.Actions; }

	/** Get action count */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Actions")
	int32 GetActionCount() const { return CurrentConfiguration.Actions.Num(); }

	/** Get action by index */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Actions")
	bool GetActionAtIndex(int32 Index, FInputActionDefinition& OutAction) const;

	/** Get action by name */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Actions")
	bool GetActionByName(FName ActionName, FInputActionDefinition& OutAction) const;

	/** Add a new action */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Actions")
	void AddAction(const FInputActionDefinition& Action);

	/** Update an existing action */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Actions")
	bool UpdateAction(FName ActionName, const FInputActionDefinition& UpdatedAction);

	/** Remove an action by name */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Actions")
	bool RemoveAction(FName ActionName);

	/** Remove action at index */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Actions")
	bool RemoveActionAtIndex(int32 Index);

	/** Clear all actions */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Actions")
	void ClearAllActions();

	/** Duplicate an action */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Actions")
	bool DuplicateAction(FName ActionName, FName NewName);

	/** Get unique categories from all actions */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Actions")
	TArray<FString> GetCategories() const { return CurrentConfiguration.GetCategories(); }

	/** Get actions in a specific category */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Actions")
	TArray<FInputActionDefinition> GetActionsInCategory(const FString& Category) const;

	// ==================== Touch Controls ====================

	/** Get all touch controls */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Touch")
	TArray<FTouchControlDefinition> GetAllTouchControls() const { return CurrentConfiguration.TouchControls; }

	/** Add a touch control */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	void AddTouchControl(const FTouchControlDefinition& Control);

	/** Remove a touch control by name */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	bool RemoveTouchControl(FName ControlName);

	/** Update touch control */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	bool UpdateTouchControl(FName ControlName, const FTouchControlDefinition& UpdatedControl);

	/** Get gyro configuration */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Touch")
	FGyroConfiguration GetGyroConfiguration() const { return CurrentConfiguration.GyroConfig; }

	/** Set gyro configuration */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
	void SetGyroConfiguration(const FGyroConfiguration& Config);

	// ==================== Generation ====================

	/** Generate all assets based on current configuration */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	void GenerateAssets();

	/** Generate only Input Actions */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	void GenerateInputActions();

	/** Generate only Mapping Contexts */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	void GenerateMappingContexts();

	/** Generate touch control widgets */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	void GenerateTouchControls();

	/** Preview what will be generated (returns list of asset paths) */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	TArray<FString> PreviewGeneration() const;

	// ==================== Persistence ====================

	/** Save configuration to JSON file */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Persistence")
	bool SaveConfiguration(const FString& FilePath);

	/** Load configuration from JSON file */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Persistence")
	bool LoadConfiguration(const FString& FilePath);

	/** Get default save path */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Persistence")
	FString GetDefaultConfigPath() const;

	/** Export configuration to clipboard as JSON */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Persistence")
	void ExportToClipboard();

	/** Import configuration from clipboard JSON */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Persistence")
	bool ImportFromClipboard();

	// ==================== Validation ====================

	/** Validate the current configuration */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Validation")
	bool ValidateConfiguration(TArray<FString>& OutErrors) const;

	/** Check if an action name is valid and unique */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Validation")
	bool IsValidActionName(FName ActionName, FName ExcludeName = NAME_None) const;

	// ==================== Events ====================

	/** Called when configuration is updated */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnConfigurationUpdated OnConfigurationUpdated;

	/** Called when LLM parsing completes */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnLLMParseComplete OnLLMParseComplete;

	/** Called when asset generation completes */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnGenerationComplete OnGenerationComplete;

	/** Called when an action is selected in the UI */
	UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Events")
	FOnActionSelected OnActionSelected;

	// ==================== UI State ====================

	/** Currently selected action index (-1 if none) */
	UPROPERTY(BlueprintReadWrite, Category = "Input Streamliner|UI")
	int32 SelectedActionIndex = -1;

	/** Select an action by index */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|UI")
	void SelectAction(int32 Index);

	/** Get the selected action */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|UI")
	bool GetSelectedAction(FInputActionDefinition& OutAction) const;

protected:
	/** The current configuration being edited */
	UPROPERTY(BlueprintReadOnly, Category = "Input Streamliner")
	FInputStreamlinerConfiguration CurrentConfiguration;

	/** LLM Parser instance */
	UPROPERTY()
	TObjectPtr<ULLMIntentParser> LLMParser;

	/** Asset Generator instance */
	UPROPERTY()
	TObjectPtr<UInputAssetGenerator> AssetGenerator;

	/** Called when LLM parsing completes */
	UFUNCTION()
	void HandleLLMParseComplete(bool bSuccess, const FString& ErrorMessage);

	/** Called when LLM connection test completes */
	UFUNCTION()
	void HandleConnectionTestComplete(bool bSuccess, const FString& ErrorMessage);

	/** Broadcast configuration update */
	void BroadcastConfigurationUpdate();

	/** Build the UI programmatically */
	void BuildUI();

	/** Refresh the actions list display */
	void RefreshActionsList();

	/** Update status text */
	void SetStatusText(const FString& Text, FLinearColor Color = FLinearColor::White);

	// UI Button handlers
	UFUNCTION()
	void OnParseButtonClicked();

	UFUNCTION()
	void OnAddActionButtonClicked();

	UFUNCTION()
	void OnGenerateButtonClicked();

	UFUNCTION()
	void OnTestConnectionButtonClicked();

	UFUNCTION()
	void OnClearAllButtonClicked();

	UFUNCTION()
	void OnDeleteGeneratedAssetsClicked();

	UFUNCTION()
	void OnModelSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	/** Fetch available models from Ollama */
	void RefreshModelList();

	/** Handle model list response from Ollama */
	void OnModelListReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// ==================== UI Elements ====================

	/** Container from Blueprint - add a VerticalBox named "ContentContainer" in the Blueprint designer */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ContentContainer;

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TObjectPtr<UMultiLineEditableTextBox> DescriptionInput;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ActionsListBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> ProjectPrefixInput;

	UPROPERTY()
	TObjectPtr<UScrollBox> ActionsScrollBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> ModelDropdown;
};
