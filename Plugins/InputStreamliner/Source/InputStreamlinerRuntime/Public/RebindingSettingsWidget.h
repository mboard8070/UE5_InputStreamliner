// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputRebindingManager.h"
#include "RebindingSettingsWidget.generated.h"

class UVerticalBox;
class UButton;
class UTextBlock;
class USlider;
class UCheckBox;
class UScrollBox;
class UInputAction;

/**
 * Individual row for displaying and rebinding a single action
 */
UCLASS()
class INPUTSTREAMLINERRUNTIME_API URebindActionRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initialize this row with an action */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void SetupAction(UInputAction* InAction, UInputRebindingManager* InManager);

	/** Update the displayed key text */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void RefreshKeyDisplay();

	/** Set whether this row is currently rebinding */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void SetRebindingState(bool bIsRebinding);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnRebindClicked();

	UFUNCTION()
	void OnResetClicked();

	UFUNCTION()
	void OnRebindComplete(UInputAction* Action, FKey NewKey);

	/** The action this row represents */
	UPROPERTY(BlueprintReadOnly, Category = "Rebinding")
	TObjectPtr<UInputAction> Action;

	/** Reference to the rebinding manager */
	UPROPERTY()
	TObjectPtr<UInputRebindingManager> RebindingManager;

	/** Action name text */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionNameText;

	/** Current key binding text */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyBindingText;

	/** Button to start rebinding */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RebindButton;

	/** Button to reset to default */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;

	/** Text on the rebind button */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RebindButtonText;

private:
	/** Programmatically created widgets (when not using Blueprint) */
	void CreateWidgets();

	bool bWidgetsCreated = false;
};

/**
 * Complete settings widget for player input rebinding
 * Can be used as-is or subclassed in Blueprint for custom styling
 */
UCLASS(Blueprintable, BlueprintType)
class INPUTSTREAMLINERRUNTIME_API URebindingSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Register an action to appear in the rebinding list */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void RegisterAction(UInputAction* Action, const TArray<FKey>& DefaultBindings);

	/** Register multiple actions at once */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void RegisterActions(const TMap<UInputAction*, FKey>& ActionsAndDefaults);

	/** Set the mapping context to modify at runtime */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void SetMappingContext(UInputMappingContext* Context);

	/** Refresh all displayed bindings */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void RefreshAllBindings();

	/** Reset all bindings to defaults */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void ResetAllToDefaults();

	/** Save current bindings */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void SaveBindings();

	/** Called when any binding changes */
	UPROPERTY(BlueprintAssignable, Category = "Rebinding")
	FOnRebindComplete OnBindingChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnRebindComplete(UInputAction* Action, FKey NewKey);

	UFUNCTION()
	void OnAnyKeyPressed(FKey Key);

	UFUNCTION()
	void OnBindingConflict(UInputAction* ExistingAction, FKey ConflictingKey);

	UFUNCTION()
	void OnMouseSensitivityChanged(float Value);

	UFUNCTION()
	void OnGamepadSensitivityChanged(float Value);

	UFUNCTION()
	void OnInvertYChanged(bool bIsChecked);

	UFUNCTION()
	void OnResetAllClicked();

	UFUNCTION()
	void OnSaveClicked();

	UFUNCTION()
	void OnCancelClicked();

	/** Container for action rows */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ActionsScrollBox;

	/** Container for action rows (alternative to ScrollBox) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ActionsContainer;

	/** Mouse sensitivity slider */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MouseSensitivitySlider;

	/** Mouse sensitivity value text */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MouseSensitivityText;

	/** Gamepad sensitivity slider */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> GamepadSensitivitySlider;

	/** Gamepad sensitivity value text */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GamepadSensitivityText;

	/** Invert Y axis checkbox */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> InvertYCheckBox;

	/** Reset all button */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetAllButton;

	/** Save button */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveButton;

	/** Cancel button */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;

	/** Status text for feedback */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Class to use for action rows (can be overridden in Blueprint) */
	UPROPERTY(EditDefaultsOnly, Category = "Rebinding")
	TSubclassOf<URebindActionRow> ActionRowClass;

private:
	/** Create all widgets programmatically */
	void CreateWidgets();

	/** Create a single action row */
	URebindActionRow* CreateActionRow(UInputAction* Action);

	/** Update status text */
	void SetStatus(const FString& Message);

	/** Get the rebinding manager */
	UInputRebindingManager* GetRebindingManager() const;

	/** Map of actions to their row widgets */
	UPROPERTY()
	TMap<TObjectPtr<UInputAction>, TObjectPtr<URebindActionRow>> ActionRows;

	/** Cached reference to rebinding manager */
	UPROPERTY()
	TObjectPtr<UInputRebindingManager> CachedManager;

	/** Currently rebinding action (for UI feedback) */
	UPROPERTY()
	TObjectPtr<UInputAction> CurrentlyRebindingAction;

	bool bWidgetsCreated = false;
};
