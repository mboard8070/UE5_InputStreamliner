// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class InputStreamliner : ModuleRules
{
	public InputStreamliner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"InputStreamlinerRuntime",
			"ApplicationCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"Blutility",
			"HTTP",
			"Json",
			"JsonUtilities",
			"AssetTools",
			"ContentBrowser",
			"EditorSubsystem",
			"ToolMenus",
			"Projects"
		});
	}
}
