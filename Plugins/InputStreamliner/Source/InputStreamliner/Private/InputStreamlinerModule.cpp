// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputStreamlinerModule.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FInputStreamlinerModule"

DEFINE_LOG_CATEGORY(LogInputStreamliner);

void FInputStreamlinerModule::StartupModule()
{
	UE_LOG(LogInputStreamliner, Log, TEXT("InputStreamliner module starting up"));

	// Register menu extensions after ToolMenus is initialized
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
		this, &FInputStreamlinerModule::RegisterMenuExtensions));
}

void FInputStreamlinerModule::ShutdownModule()
{
	UE_LOG(LogInputStreamliner, Log, TEXT("InputStreamliner module shutting down"));

	UnregisterMenuExtensions();

	// Unregister startup callback
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FInputStreamlinerModule::RegisterMenuExtensions()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// Register the Input Streamliner menu entry under Tools
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection("InputStreamliner");

	Section.AddMenuEntry(
		"OpenInputStreamliner",
		LOCTEXT("OpenInputStreamlinerLabel", "Input Streamliner"),
		LOCTEXT("OpenInputStreamlinerTooltip", "Open the Input Streamliner tool to configure multiplatform input"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			UE_LOG(LogInputStreamliner, Log, TEXT("Input Streamliner menu item clicked"));
			// TODO: Open the Input Streamliner editor utility widget
		}))
	);
}

void FInputStreamlinerModule::UnregisterMenuExtensions()
{
	// Menu extensions are automatically cleaned up by FToolMenuOwnerScoped
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInputStreamlinerModule, InputStreamliner)
