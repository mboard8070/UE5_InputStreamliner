// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputStreamlinerConfiguration.h"
#include "InputAssetGenerator.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Generates Unreal Engine input assets from InputStreamliner configuration
 */
UCLASS(BlueprintType)
class INPUTSTREAMLINER_API UInputAssetGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Generate all input assets from the configuration
	 * @param Config The input configuration to generate from
	 * @param OutCreatedAssets Array of created asset objects
	 * @return True if generation succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	bool GenerateInputAssets(const FInputStreamlinerConfiguration& Config, TArray<UObject*>& OutCreatedAssets);

	/**
	 * Generate a single Input Action asset
	 * @param Definition The action definition
	 * @param Path The content path to create the asset in
	 * @return The created Input Action, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	UInputAction* GenerateInputAction(const FInputActionDefinition& Definition, const FString& Path);

	/**
	 * Generate an Input Mapping Context for a specific platform
	 * @param Platform The target platform
	 * @param Actions Array of action definitions to include
	 * @param Path The content path to create the asset in
	 * @param Config The full configuration (for accessing generated actions)
	 * @return The created Mapping Context, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	UInputMappingContext* GenerateMappingContext(
		ETargetPlatform Platform,
		const TArray<FInputActionDefinition>& Actions,
		const FString& Path,
		const FInputStreamlinerConfiguration& Config);

	/**
	 * Delete all previously generated input assets
	 * @param Config The configuration containing paths
	 * @return Number of assets deleted
	 */
	UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
	int32 CleanupGeneratedAssets(const FInputStreamlinerConfiguration& Config);

	/**
	 * Check if an asset already exists at the given path
	 */
	UFUNCTION(BlueprintPure, Category = "Input Streamliner|Generation")
	bool DoesAssetExist(const FString& AssetPath) const;

private:
	/** Configure triggers and modifiers on an Input Action based on the definition */
	void ConfigureActionTriggers(UInputAction* Action, const FInputActionDefinition& Definition);

	/** Add a key mapping to a mapping context */
	void AddMappingToContext(
		UInputMappingContext* Context,
		UInputAction* Action,
		const FKeyBindingDefinition& Binding,
		const FInputActionDefinition& ActionDef);

	/** Get a platform-friendly name for file naming */
	static FString GetPlatformName(ETargetPlatform Platform);

	/** Map of generated actions by name for quick lookup */
	UPROPERTY(Transient)
	TMap<FName, UInputAction*> GeneratedActions;
};
