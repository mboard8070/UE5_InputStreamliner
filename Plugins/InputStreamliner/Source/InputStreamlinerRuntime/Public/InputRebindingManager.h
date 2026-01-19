// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InputAction.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputRebindingManager.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRebindComplete, UInputAction*, Action, FKey, NewKey);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnyKeyPressed, FKey, Key);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBindingConflict, UInputAction*, ExistingAction, FKey, ConflictingKey);

/**
 * Data structure for saving player input bindings
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINERRUNTIME_API FActionBindingSave
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName ActionName;

	UPROPERTY(SaveGame)
	TArray<FKey> Keys;
};

/**
 * Complete save data for all input bindings and settings
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINERRUNTIME_API FInputBindingSaveData
{
	GENERATED_BODY()

	/** Save data version for migration */
	UPROPERTY(SaveGame)
	int32 Version = 1;

	/** Per-action key bindings */
	UPROPERTY(SaveGame)
	TArray<FActionBindingSave> Bindings;

	/** Mouse look sensitivity */
	UPROPERTY(SaveGame)
	float MouseSensitivity = 1.0f;

	/** Gamepad look sensitivity */
	UPROPERTY(SaveGame)
	float GamepadSensitivity = 1.0f;

	/** Gyroscope sensitivity (mobile) */
	UPROPERTY(SaveGame)
	float GyroSensitivity = 1.0f;

	/** Invert Y axis for look */
	UPROPERTY(SaveGame)
	bool bInvertY = false;

	/** Whether gyroscope is enabled */
	UPROPERTY(SaveGame)
	bool bGyroEnabled = false;

	/** Custom touch control positions (control name -> screen position) */
	UPROPERTY(SaveGame)
	TMap<FName, FVector2D> TouchControlPositions;
};

/**
 * Runtime subsystem for managing input rebinding
 * Persists player bindings and handles the rebinding UI flow
 */
UCLASS(BlueprintType)
class INPUTSTREAMLINERRUNTIME_API UInputRebindingManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	// Rebinding Flow

	/** Start listening for a new key binding for an action */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void StartRebinding(UInputAction* Action);

	/** Cancel the current rebinding operation */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void CancelRebinding();

	/** Check if currently waiting for a key press */
	UFUNCTION(BlueprintPure, Category = "Rebinding")
	bool IsRebindingInProgress() const { return PendingRebindAction != nullptr; }

	/** Get the action currently being rebound */
	UFUNCTION(BlueprintPure, Category = "Rebinding")
	UInputAction* GetPendingRebindAction() const { return PendingRebindAction; }

	// Binding Management

	/** Get current bindings for an action */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	TArray<FKey> GetBindingsForAction(UInputAction* Action) const;

	/** Apply a new binding to an action */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	bool ApplyBinding(UInputAction* Action, FKey NewKey, int32 BindingIndex = 0);

	/** Remove a binding from an action */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	bool RemoveBinding(UInputAction* Action, int32 BindingIndex);

	/** Check for binding conflicts */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	bool HasConflict(UInputAction* Action, FKey Key, UInputAction*& OutConflictingAction) const;

	/** Swap bindings between two actions (for conflict resolution) */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void SwapBindings(UInputAction* ActionA, UInputAction* ActionB, FKey Key);

	/** Reset an action to its default binding */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void ResetToDefault(UInputAction* Action);

	/** Reset all actions to default bindings */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void ResetAllToDefaults();

	// Sensitivity Settings

	UFUNCTION(BlueprintCallable, Category = "Rebinding|Sensitivity")
	void SetMouseSensitivity(float Sensitivity);

	UFUNCTION(BlueprintPure, Category = "Rebinding|Sensitivity")
	float GetMouseSensitivity() const { return SaveData.MouseSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Rebinding|Sensitivity")
	void SetGamepadSensitivity(float Sensitivity);

	UFUNCTION(BlueprintPure, Category = "Rebinding|Sensitivity")
	float GetGamepadSensitivity() const { return SaveData.GamepadSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Rebinding|Sensitivity")
	void SetGyroSensitivity(float Sensitivity);

	UFUNCTION(BlueprintPure, Category = "Rebinding|Sensitivity")
	float GetGyroSensitivity() const { return SaveData.GyroSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Rebinding|Sensitivity")
	void SetInvertY(bool bInvert);

	UFUNCTION(BlueprintPure, Category = "Rebinding|Sensitivity")
	bool GetInvertY() const { return SaveData.bInvertY; }

	// Persistence

	/** Save bindings to local storage */
	UFUNCTION(BlueprintCallable, Category = "Rebinding|Persistence")
	bool SaveBindings();

	/** Load bindings from local storage */
	UFUNCTION(BlueprintCallable, Category = "Rebinding|Persistence")
	bool LoadBindings();

	/** Get the save slot name */
	UFUNCTION(BlueprintPure, Category = "Rebinding|Persistence")
	FString GetSaveSlotName() const { return TEXT("InputBindings"); }

	// Registration

	/** Register an action with its default bindings (call during game setup) */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void RegisterAction(UInputAction* Action, const TArray<FKey>& DefaultBindings);

	// Events

	/** Called when rebinding completes successfully */
	UPROPERTY(BlueprintAssignable, Category = "Rebinding|Events")
	FOnRebindComplete OnRebindComplete;

	/** Called when any key is pressed during rebinding (for UI feedback) */
	UPROPERTY(BlueprintAssignable, Category = "Rebinding|Events")
	FOnAnyKeyPressed OnAnyKeyPressed;

	/** Called when a binding conflict is detected */
	UPROPERTY(BlueprintAssignable, Category = "Rebinding|Events")
	FOnBindingConflict OnBindingConflict;

	/** Get the active mapping context for a player */
	UFUNCTION(BlueprintCallable, Category = "Rebinding")
	void SetMappingContext(UInputMappingContext* Context);

	/** Get the registered mapping context */
	UFUNCTION(BlueprintPure, Category = "Rebinding")
	UInputMappingContext* GetMappingContext() const { return ActiveMappingContext; }

	// Allow input processor to access private members
	friend class FRebindInputProcessor;

private:
	/** Handle key input during rebinding */
	bool HandleKeyDown(const FKeyEvent& KeyEvent);

	/** Handle gamepad input during rebinding */
	bool HandleAnalogInput(const FAnalogInputEvent& AnalogEvent);

	/** Apply loaded bindings to the input system */
	void ApplyLoadedBindings();

	/** Apply a single binding change to the mapping context */
	void ApplyBindingToMappingContext(UInputAction* Action, FKey OldKey, FKey NewKey);

	/** Get Enhanced Input subsystem for local player */
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;

	/** The action currently being rebound */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PendingRebindAction;

	/** Index of the binding being changed */
	int32 PendingBindingIndex = 0;

	/** Stored default bindings for reset functionality (not UPROPERTY - TMap<TArray> not supported) */
	TMap<TObjectPtr<UInputAction>, TArray<FKey>> DefaultBindings;

	/** Current custom bindings (not UPROPERTY - TMap<TArray> not supported) */
	TMap<TObjectPtr<UInputAction>, TArray<FKey>> CurrentBindings;

	/** Save data */
	UPROPERTY(Transient)
	FInputBindingSaveData SaveData;

	/** The active mapping context to modify */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveMappingContext;

	/** Input processor for capturing rebind keys */
	TSharedPtr<class FRebindInputProcessor> RebindInputProcessor;
};

/**
 * Input processor that captures key presses during rebinding
 */
class FRebindInputProcessor : public IInputProcessor
{
public:
	FRebindInputProcessor(UInputRebindingManager* InManager) : Manager(InManager) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return false; }
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return false; }
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return false; }
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override;

private:
	TWeakObjectPtr<UInputRebindingManager> Manager;
};
