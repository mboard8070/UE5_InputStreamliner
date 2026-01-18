// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionDefinition.h"
#include "TouchControlDefinition.h"
#include "InputStreamlinerConfiguration.generated.h"

/**
 * Code generation output type
 */
UENUM(BlueprintType)
enum class ECodeGenerationType : uint8
{
	Blueprint		UMETA(DisplayName = "Blueprint Only"),
	CPlusPlus		UMETA(DisplayName = "C++ Only"),
	Both			UMETA(DisplayName = "Both C++ and Blueprint")
};

/**
 * Complete configuration for Input Streamliner
 * Contains all input actions, touch controls, and generation settings
 */
USTRUCT(BlueprintType)
struct INPUTSTREAMLINER_API FInputStreamlinerConfiguration
{
	GENERATED_BODY()

	/** Project prefix for generated asset names */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FString ProjectPrefix = TEXT("Game");

	/** All defined input actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<FInputActionDefinition> Actions;

	/** Mobile touch control definitions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch")
	TArray<FTouchControlDefinition> TouchControls;

	/** Gyroscope configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch")
	FGyroConfiguration GyroConfig;

	// Generation Options

	/** Type of code to generate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	ECodeGenerationType CodeGenType = ECodeGenerationType::Blueprint;

	/** Whether to generate a rebinding settings UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateRebindingUI = true;

	/** Whether to generate touch control widgets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateTouchControls = true;

	/** Whether to generate platform-specific mapping contexts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateMappingContexts = true;

	// Output Paths

	/** Path for generated Input Action assets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paths")
	FString InputActionsPath = TEXT("/Game/Input/Actions");

	/** Path for generated Input Mapping Context assets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paths")
	FString MappingContextsPath = TEXT("/Game/Input/Contexts");

	/** Path for generated UI widgets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paths")
	FString WidgetsPath = TEXT("/Game/UI/Input");

	/** Path for generated C++ code (relative to Source folder) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paths")
	FString GeneratedCodePath = TEXT("Input");

	// LLM Configuration

	/** Ollama endpoint URL */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM")
	FString LLMEndpointURL = TEXT("http://localhost");

	/** Ollama endpoint port */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM")
	int32 LLMEndpointPort = 11434;

	/** Model name to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM")
	FString LLMModelName = TEXT("nemotron:8b");

	FInputStreamlinerConfiguration()
		: ProjectPrefix(TEXT("Game"))
		, CodeGenType(ECodeGenerationType::Blueprint)
		, bGenerateRebindingUI(true)
		, bGenerateTouchControls(true)
		, bGenerateMappingContexts(true)
		, InputActionsPath(TEXT("/Game/Input/Actions"))
		, MappingContextsPath(TEXT("/Game/Input/Contexts"))
		, WidgetsPath(TEXT("/Game/UI/Input"))
		, GeneratedCodePath(TEXT("Input"))
		, LLMEndpointURL(TEXT("http://localhost"))
		, LLMEndpointPort(11434)
		, LLMModelName(TEXT("nemotron:8b"))
	{
	}

	/** Find an action by name */
	FInputActionDefinition* FindAction(FName ActionName)
	{
		return Actions.FindByPredicate([ActionName](const FInputActionDefinition& Action)
		{
			return Action.ActionName == ActionName;
		});
	}

	/** Find an action by name (const version) */
	const FInputActionDefinition* FindAction(FName ActionName) const
	{
		return Actions.FindByPredicate([ActionName](const FInputActionDefinition& Action)
		{
			return Action.ActionName == ActionName;
		});
	}

	/** Check if an action name exists */
	bool HasAction(FName ActionName) const
	{
		return FindAction(ActionName) != nullptr;
	}

	/** Get all action names */
	TArray<FName> GetActionNames() const
	{
		TArray<FName> Names;
		for (const FInputActionDefinition& Action : Actions)
		{
			Names.Add(Action.ActionName);
		}
		return Names;
	}

	/** Get actions in a specific category */
	TArray<FInputActionDefinition> GetActionsInCategory(const FString& Category) const
	{
		TArray<FInputActionDefinition> Result;
		for (const FInputActionDefinition& Action : Actions)
		{
			if (Action.Category == Category)
			{
				Result.Add(Action);
			}
		}
		return Result;
	}

	/** Get all unique categories */
	TArray<FString> GetCategories() const
	{
		TSet<FString> CategorySet;
		for (const FInputActionDefinition& Action : Actions)
		{
			CategorySet.Add(Action.Category);
		}

		TArray<FString> Categories = CategorySet.Array();
		Categories.Sort();
		return Categories;
	}
};
