// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Ashen_CathedralEditorTarget : TargetRules
{
	public Ashen_CathedralEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.AddRange(new string[]
		{
			"Ashen_Cathedral",
			"Ashen_CathedralEditor"
		});
	}
}
