// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionDefinition.generated.h"

/**
 * Defines the type of value an input action produces
 */
UENUM(BlueprintType)
enum class EInputActionType : uint8
{
	Bool		UMETA(DisplayName = "Button (Bool)"),
	Axis1D		UMETA(DisplayName = "1D Axis (Float)"),
	Axis2D		UMETA(DisplayName = "2D Axis (Vector2D)"),
	Axis3D		UMETA(DisplayName = "3D Axis (Vector)")
};

/**
 * Target platforms for input configuration
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETargetPlatform : uint8
{
	None			= 0			UMETA(Hidden),
	PC_Keyboard		= 1 << 0	UMETA(DisplayName = "PC (Keyboard/Mouse)"),
	PC_Gamepad		= 1 << 1	UMETA(DisplayName = "PC (Gamepad)"),
	Mac				= 1 << 2	UMETA(DisplayName = "Mac"),
	iOS				= 1 << 3	UMETA(DisplayName = "iOS"),
	Android			= 1 << 4	UMETA(DisplayName = "Android"),
	All				= 0xFF		UMETA(DisplayName = "All Platforms")
};
ENUM_CLASS_FLAGS(ETargetPlatform);

/**
 * Trigger type for input actions
 */
UENUM(BlueprintType)
enum class EInputTriggerType : uint8
{
	Pressed		UMETA(DisplayName = "Pressed"),
	Released	UMETA(DisplayName = "Released"),
	Hold		UMETA(DisplayName = "Hold"),
	Tap			UMETA(DisplayName = "Tap"),
	DoubleTap	UMETA(DisplayName = "Double Tap")
};

/**
 * A single key binding with optional modifiers and triggers
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FKeyBindingDefinition
{
	GENERATED_BODY()

	/** The primary key for this binding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FKey Key;

	/** Modifier keys required (Shift, Ctrl, Alt) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	TArray<FKey> Modifiers;

	/** The trigger type for this binding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	EInputTriggerType TriggerType = EInputTriggerType::Pressed;

	/** For axis inputs, which axis direction this key represents */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FString AxisMapping;

	FKeyBindingDefinition()
		: Key(EKeys::Invalid)
		, TriggerType(EInputTriggerType::Pressed)
	{
	}

	FKeyBindingDefinition(FKey InKey)
		: Key(InKey)
		, TriggerType(EInputTriggerType::Pressed)
	{
	}
};

/**
 * Platform-specific binding configuration
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FPlatformBindingConfig
{
	GENERATED_BODY()

	/** Key bindings for this platform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	TArray<FKeyBindingDefinition> Bindings;

	/** For mobile: the type of touch control to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
	FString TouchControlType;
};

/**
 * Complete definition of an input action
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FInputActionDefinition
{
	GENERATED_BODY()

	/** Unique identifier for this action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FName ActionName;

	/** Human-readable display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FString DisplayName;

	/** Description of what this action does */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FString Description;

	/** The type of value this action produces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EInputActionType ActionType = EInputActionType::Bool;

	/** Target platforms for this action (bitmask) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (Bitmask, BitmaskEnum = "/Script/InputStreamliner.ETargetPlatform"))
	int32 TargetPlatforms = static_cast<int32>(ETargetPlatform::All);

	/** Whether players can rebind this action at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	bool bAllowRebinding = true;

	/** Category for UI grouping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FString Category = TEXT("General");

	/** Platform-specific bindings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	TMap<ETargetPlatform, FPlatformBindingConfig> PlatformBindings;

	FInputActionDefinition()
		: ActionType(EInputActionType::Bool)
		, TargetPlatforms(static_cast<int32>(ETargetPlatform::All))
		, bAllowRebinding(true)
		, Category(TEXT("General"))
	{
	}

	/** Check if this action targets a specific platform */
	bool TargetsPlatform(ETargetPlatform Platform) const
	{
		return (TargetPlatforms & static_cast<int32>(Platform)) != 0;
	}
};
