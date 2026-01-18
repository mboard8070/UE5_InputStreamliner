// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputAssetGenerator.h"
#include "InputStreamlinerModule.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

bool UInputAssetGenerator::GenerateInputAssets(const FInputStreamlinerConfiguration& Config, TArray<UObject*>& OutCreatedAssets)
{
	GeneratedActions.Empty();
	OutCreatedAssets.Empty();

	UE_LOG(LogInputStreamliner, Log, TEXT("Starting asset generation for %d actions"), Config.Actions.Num());

	// Generate Input Actions
	for (const FInputActionDefinition& ActionDef : Config.Actions)
	{
		UInputAction* Action = GenerateInputAction(ActionDef, Config.InputActionsPath);
		if (Action)
		{
			GeneratedActions.Add(ActionDef.ActionName, Action);
			OutCreatedAssets.Add(Action);
		}
		else
		{
			UE_LOG(LogInputStreamliner, Error, TEXT("Failed to generate Input Action: %s"), *ActionDef.ActionName.ToString());
		}
	}

	// Generate Mapping Contexts for each platform
	if (Config.bGenerateMappingContexts)
	{
		TArray<ETargetPlatform> Platforms = {
			ETargetPlatform::PC_Keyboard,
			ETargetPlatform::PC_Gamepad,
			ETargetPlatform::Mac,
			ETargetPlatform::iOS,
			ETargetPlatform::Android
		};

		for (ETargetPlatform Platform : Platforms)
		{
			// Filter actions that target this platform
			TArray<FInputActionDefinition> PlatformActions;
			for (const FInputActionDefinition& ActionDef : Config.Actions)
			{
				if (ActionDef.TargetsPlatform(Platform))
				{
					PlatformActions.Add(ActionDef);
				}
			}

			if (PlatformActions.Num() > 0)
			{
				UInputMappingContext* Context = GenerateMappingContext(Platform, PlatformActions, Config.MappingContextsPath, Config);
				if (Context)
				{
					OutCreatedAssets.Add(Context);
				}
			}
		}
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Asset generation complete. Created %d assets."), OutCreatedAssets.Num());
	return OutCreatedAssets.Num() > 0;
}

UInputAction* UInputAssetGenerator::GenerateInputAction(const FInputActionDefinition& Definition, const FString& Path)
{
	FString AssetName = FString::Printf(TEXT("IA_%s"), *Definition.ActionName.ToString());
	FString PackagePath = Path / AssetName;

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to create package: %s"), *PackagePath);
		return nullptr;
	}

	Package->FullyLoad();

	// Create Input Action
	UInputAction* InputAction = NewObject<UInputAction>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!InputAction)
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("Failed to create Input Action object"));
		return nullptr;
	}

	// Set value type
	switch (Definition.ActionType)
	{
	case EInputActionType::Bool:
		InputAction->ValueType = EInputActionValueType::Boolean;
		break;
	case EInputActionType::Axis1D:
		InputAction->ValueType = EInputActionValueType::Axis1D;
		break;
	case EInputActionType::Axis2D:
		InputAction->ValueType = EInputActionValueType::Axis2D;
		break;
	case EInputActionType::Axis3D:
		InputAction->ValueType = EInputActionValueType::Axis3D;
		break;
	}

	// Configure triggers
	ConfigureActionTriggers(InputAction, Definition);

	// Mark package dirty and register with asset registry
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(InputAction);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

	if (!UPackage::SavePackage(Package, InputAction, *PackageFileName, SaveArgs))
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Failed to save package: %s"), *PackageFileName);
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Generated Input Action: %s"), *AssetName);
	return InputAction;
}

UInputMappingContext* UInputAssetGenerator::GenerateMappingContext(
	ETargetPlatform Platform,
	const TArray<FInputActionDefinition>& Actions,
	const FString& Path,
	const FInputStreamlinerConfiguration& Config)
{
	FString PlatformName = GetPlatformName(Platform);
	FString AssetName = FString::Printf(TEXT("IMC_%s"), *PlatformName);
	FString PackagePath = Path / AssetName;

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return nullptr;
	}

	Package->FullyLoad();

	// Create Mapping Context
	UInputMappingContext* Context = NewObject<UInputMappingContext>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!Context)
	{
		return nullptr;
	}

	// Add mappings for each action
	for (const FInputActionDefinition& ActionDef : Actions)
	{
		UInputAction** FoundAction = GeneratedActions.Find(ActionDef.ActionName);
		if (!FoundAction || !*FoundAction)
		{
			UE_LOG(LogInputStreamliner, Warning, TEXT("Could not find generated action: %s"), *ActionDef.ActionName.ToString());
			continue;
		}

		// Get bindings for this platform
		const FPlatformBindingConfig* PlatformBinding = ActionDef.PlatformBindings.Find(Platform);
		if (!PlatformBinding)
		{
			continue;
		}

		// Add each key binding
		for (const FKeyBindingDefinition& Binding : PlatformBinding->Bindings)
		{
			AddMappingToContext(Context, *FoundAction, Binding, ActionDef);
		}
	}

	// Save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Context);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Context, *PackageFileName, SaveArgs);

	UE_LOG(LogInputStreamliner, Log, TEXT("Generated Mapping Context: %s"), *AssetName);
	return Context;
}

int32 UInputAssetGenerator::CleanupGeneratedAssets(const FInputStreamlinerConfiguration& Config)
{
	// TODO: Implement asset cleanup
	UE_LOG(LogInputStreamliner, Log, TEXT("Asset cleanup not yet implemented"));
	return 0;
}

bool UInputAssetGenerator::DoesAssetExist(const FString& AssetPath) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	return AssetData.IsValid();
}

void UInputAssetGenerator::ConfigureActionTriggers(UInputAction* Action, const FInputActionDefinition& Definition)
{
	// Default triggers are typically fine, but we can add specific ones based on the definition
	// For now, we'll let the mapping context handle triggers
}

void UInputAssetGenerator::AddMappingToContext(
	UInputMappingContext* Context,
	UInputAction* Action,
	const FKeyBindingDefinition& Binding,
	const FInputActionDefinition& ActionDef)
{
	if (!Binding.Key.IsValid())
	{
		return;
	}

	FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Binding.Key);

	// Add trigger based on binding type
	switch (Binding.TriggerType)
	{
	case EInputTriggerType::Hold:
		{
			UInputTriggerHold* HoldTrigger = NewObject<UInputTriggerHold>(Context);
			Mapping.Triggers.Add(HoldTrigger);
		}
		break;
	case EInputTriggerType::Tap:
		{
			UInputTriggerTap* TapTrigger = NewObject<UInputTriggerTap>(Context);
			Mapping.Triggers.Add(TapTrigger);
		}
		break;
	case EInputTriggerType::Pressed:
	default:
		{
			UInputTriggerPressed* PressedTrigger = NewObject<UInputTriggerPressed>(Context);
			Mapping.Triggers.Add(PressedTrigger);
		}
		break;
	}

	// Add axis modifiers if this is an axis mapping
	if (!Binding.AxisMapping.IsEmpty() && ActionDef.ActionType == EInputActionType::Axis2D)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);

		if (Binding.AxisMapping.Contains(TEXT("X")))
		{
			if (Binding.AxisMapping.Contains(TEXT("-")))
			{
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Context);
				Mapping.Modifiers.Add(Negate);
			}
		}
		else if (Binding.AxisMapping.Contains(TEXT("Y")))
		{
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);

			if (Binding.AxisMapping.Contains(TEXT("-")))
			{
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Context);
				Mapping.Modifiers.Add(Negate);
			}
		}
	}
}

FString UInputAssetGenerator::GetPlatformName(ETargetPlatform Platform)
{
	switch (Platform)
	{
	case ETargetPlatform::PC_Keyboard:
		return TEXT("PC_Keyboard");
	case ETargetPlatform::PC_Gamepad:
		return TEXT("PC_Gamepad");
	case ETargetPlatform::Mac:
		return TEXT("Mac");
	case ETargetPlatform::iOS:
		return TEXT("iOS");
	case ETargetPlatform::Android:
		return TEXT("Android");
	default:
		return TEXT("Unknown");
	}
}
