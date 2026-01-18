// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputAction.h"
#include "VirtualJoystickWidget.generated.h"

/**
 * Virtual joystick widget for touch-based movement input
 * Can be configured as fixed position or floating (appears where touched)
 */
UCLASS(BlueprintType, Blueprintable)
class INPUTSTREAMLINERRUNTIME_API UVirtualJoystickWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UVirtualJoystickWidget(const FObjectInitializer& ObjectInitializer);

	// Configuration

	/** Whether the joystick floats to the touch position or stays fixed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	bool bIsFloating = false;

	/** Dead zone as a percentage of the joystick radius (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DeadZone = 0.15f;

	/** Visual size of the joystick background in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick", meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float VisualSize = 150.0f;

	/** Size of the thumb/handle in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick", meta = (ClampMin = "20.0", ClampMax = "200.0"))
	float ThumbSize = 50.0f;

	/** Visual opacity when not being touched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IdleOpacity = 0.5f;

	/** Visual opacity when being touched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ActiveOpacity = 0.9f;

	/** Whether the joystick returns to center when released */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	bool bAutoCenter = true;

	// Input Action

	/** The Input Action this joystick controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LinkedAction;

	// State (read-only)

	/** Current joystick output value (normalized -1 to 1 per axis) */
	UPROPERTY(BlueprintReadOnly, Category = "Joystick")
	FVector2D CurrentValue;

	/** Whether the joystick is currently being touched */
	UPROPERTY(BlueprintReadOnly, Category = "Joystick")
	bool bIsActive = false;

	// Blueprint Events

	/** Called when the joystick value changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Joystick")
	void OnJoystickValueChanged(FVector2D NewValue);

	/** Called when touch starts on the joystick */
	UFUNCTION(BlueprintImplementableEvent, Category = "Joystick")
	void OnJoystickActivated();

	/** Called when touch ends on the joystick */
	UFUNCTION(BlueprintImplementableEvent, Category = "Joystick")
	void OnJoystickDeactivated();

	// Functions

	/** Reset the joystick to center position */
	UFUNCTION(BlueprintCallable, Category = "Joystick")
	void ResetToCenter();

	/** Get the current value with dead zone applied */
	UFUNCTION(BlueprintPure, Category = "Joystick")
	FVector2D GetValueWithDeadZone() const;

protected:
	// UUserWidget interface
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Update joystick position based on touch location */
	void UpdateJoystickPosition(const FVector2D& TouchPosition, const FGeometry& Geometry);

	/** Inject the input value into the Enhanced Input system */
	void InjectInputValue(const FVector2D& Value);

	/** The center position of the joystick (for floating mode) */
	FVector2D CenterPosition;

	/** The touch index currently controlling this joystick */
	int32 ActiveTouchIndex = -1;

	/** Original position for fixed joysticks */
	FVector2D OriginalPosition;
};
