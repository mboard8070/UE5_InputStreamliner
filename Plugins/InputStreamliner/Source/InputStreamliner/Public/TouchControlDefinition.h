// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TouchControlDefinition.generated.h"

/**
 * Types of touch controls that can be generated
 */
UENUM(BlueprintType)
enum class ETouchControlType : uint8
{
	None						UMETA(DisplayName = "None"),
	VirtualJoystick_Fixed		UMETA(DisplayName = "Virtual Joystick (Fixed)"),
	VirtualJoystick_Floating	UMETA(DisplayName = "Virtual Joystick (Floating)"),
	VirtualButton				UMETA(DisplayName = "Virtual Button"),
	VirtualDPad					UMETA(DisplayName = "Virtual D-Pad"),
	RadialMenu					UMETA(DisplayName = "Radial Menu"),
	TouchRegion					UMETA(DisplayName = "Touch Region"),
	GestureZone					UMETA(DisplayName = "Gesture Zone")
};

/**
 * Types of gestures that can be detected
 */
UENUM(BlueprintType)
enum class EGestureType : uint8
{
	Tap				UMETA(DisplayName = "Tap"),
	DoubleTap		UMETA(DisplayName = "Double Tap"),
	LongPress		UMETA(DisplayName = "Long Press"),
	Swipe_4Dir		UMETA(DisplayName = "Swipe (4 Directions)"),
	Swipe_8Dir		UMETA(DisplayName = "Swipe (8 Directions)"),
	Pinch			UMETA(DisplayName = "Pinch"),
	Rotate			UMETA(DisplayName = "Rotate"),
	TwoFingerTap	UMETA(DisplayName = "Two-Finger Tap")
};

/**
 * Entry in a radial menu
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FRadialMenuEntry
{
	GENERATED_BODY()

	/** The action this entry triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu")
	FName ActionName;

	/** Icon to display for this entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Label text for this entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu")
	FText Label;

	/** Tooltip text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu")
	FText Tooltip;
};

/**
 * Definition of a touch control widget
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FTouchControlDefinition
{
	GENERATED_BODY()

	/** Unique identifier for this control */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FName ControlName;

	/** Type of touch control */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	ETouchControlType ControlType = ETouchControlType::None;

	/** Screen position (0-1 normalized, origin bottom-left) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D ScreenPosition = FVector2D(0.15f, 0.3f);

	/** Size (0-1 normalized relative to screen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D Size = FVector2D(0.2f, 0.2f);

	/** The input action this control is linked to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FName LinkedActionName;

	/** Visual opacity (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 0.7f;

	/** Whether to respect device safe areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	bool bRespectSafeArea = true;

	/** Whether players can reposition this control */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	bool bAllowRepositioning = true;

	// Joystick-specific properties

	/** Dead zone for joystick input (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DeadZone = 0.15f;

	/** Whether the joystick returns to center when released */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	bool bAutoCenter = true;

	// Radial menu-specific properties

	/** Entries in the radial menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu")
	TArray<FRadialMenuEntry> RadialEntries;

	/** Whether the menu requires a hold to open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu")
	bool bRequireHoldToOpen = true;

	// Gesture-specific properties

	/** Type of gesture to detect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gesture")
	EGestureType GestureType = EGestureType::Tap;

	/** Minimum swipe distance in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gesture", meta = (ClampMin = "10.0", ClampMax = "200.0"))
	float SwipeThreshold = 50.0f;

	/** Long press duration in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gesture", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float LongPressDuration = 0.5f;

	FTouchControlDefinition()
		: ControlType(ETouchControlType::None)
		, ScreenPosition(FVector2D(0.15f, 0.3f))
		, Size(FVector2D(0.2f, 0.2f))
		, Opacity(0.7f)
		, bRespectSafeArea(true)
		, bAllowRepositioning(true)
		, DeadZone(0.15f)
		, bAutoCenter(true)
		, bRequireHoldToOpen(true)
		, GestureType(EGestureType::Tap)
		, SwipeThreshold(50.0f)
		, LongPressDuration(0.5f)
	{
	}
};

/**
 * Gyroscope configuration for mobile devices
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FGyroConfiguration
{
	GENERATED_BODY()

	/** Whether gyro input is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro")
	bool bEnabled = false;

	/** The input action to link gyro output to (typically Look or Aim) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro")
	FName LinkedActionName;

	/** Sensitivity multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float Sensitivity = 1.0f;

	/** Whether to invert horizontal axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro")
	bool bInvertHorizontal = false;

	/** Whether to invert vertical axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro")
	bool bInvertVertical = false;

	/** Action that must be active for gyro to work (e.g., Aim for ADS-only gyro) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro")
	FName ActivationAction;

	FGyroConfiguration()
		: bEnabled(false)
		, Sensitivity(1.0f)
		, bInvertHorizontal(false)
		, bInvertVertical(false)
	{
	}
};
