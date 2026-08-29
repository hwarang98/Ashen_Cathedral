// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PostProcessPresetImporterEditor : ModuleRules
{
	public PostProcessPresetImporterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"AssetTools",
			"ContentBrowser",
			"DesktopPlatform",
			"EditorFramework",
			"EditorSubsystem",
			"InputCore",
			"Json",
			"JsonUtilities",
			"LevelEditor",
			"Projects",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"ToolWidgets",
			"UnrealEd",
			"WorkspaceMenuStructure",
		});
	}
}
