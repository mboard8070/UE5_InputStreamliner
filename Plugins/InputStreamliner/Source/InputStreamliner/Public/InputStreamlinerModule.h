// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInputStreamliner, Log, All);

class FInputStreamlinerModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Singleton-like access to this module's interface.
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static FInputStreamlinerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FInputStreamlinerModule>("InputStreamliner");
	}

	/**
	 * Checks to see if this module is loaded and ready.
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("InputStreamliner");
	}

private:
	void RegisterMenuExtensions();
	void UnregisterMenuExtensions();
};
