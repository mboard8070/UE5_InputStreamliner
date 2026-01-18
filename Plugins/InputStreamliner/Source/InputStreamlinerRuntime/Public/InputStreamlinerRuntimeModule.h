// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInputStreamlinerRuntime, Log, All);

class FInputStreamlinerRuntimeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Singleton-like access to this module's interface.
	 */
	static FInputStreamlinerRuntimeModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FInputStreamlinerRuntimeModule>("InputStreamlinerRuntime");
	}

	/**
	 * Checks to see if this module is loaded and ready.
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("InputStreamlinerRuntime");
	}
};
