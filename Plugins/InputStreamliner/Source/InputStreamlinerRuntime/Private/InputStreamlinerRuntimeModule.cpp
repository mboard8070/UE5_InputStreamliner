// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputStreamlinerRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FInputStreamlinerRuntimeModule"

DEFINE_LOG_CATEGORY(LogInputStreamlinerRuntime);

void FInputStreamlinerRuntimeModule::StartupModule()
{
	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("InputStreamlinerRuntime module starting up"));
}

void FInputStreamlinerRuntimeModule::ShutdownModule()
{
	UE_LOG(LogInputStreamlinerRuntime, Log, TEXT("InputStreamlinerRuntime module shutting down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInputStreamlinerRuntimeModule, InputStreamlinerRuntime)
