using UnrealBuildTool;

public class Ashen_CathedralEditor : ModuleRules
{
	public Ashen_CathedralEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AIModule",
			"AssetRegistry",
			"ContentBrowser",
			"GameplayAbilities",
			"GameplayTags",
			"InputCore",
			"GameplayStateTreeModule",
			"StateTreeModule",
			"StateTreeEditorModule",
			"LevelEditor",
			"LevelSequence",
			"NavigationSystem",
			"Projects",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
			"PropertyEditor",
			"MotionWarping",
			"Ashen_Cathedral"
		});
	}
}
