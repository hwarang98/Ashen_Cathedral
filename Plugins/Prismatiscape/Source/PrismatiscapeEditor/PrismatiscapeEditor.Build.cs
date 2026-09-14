// © 2024, PrismaticaDev. All rights reserved.

using UnrealBuildTool;

public class PrismatiscapeEditor : ModuleRules
{
    public PrismatiscapeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
                , "Prismatiscape"
                , "Kismet"
                , "KismetCompiler"
                , "UnrealEd"
                , "GraphEditor"
                , "BlueprintGraph"
            }
        );
    }
}