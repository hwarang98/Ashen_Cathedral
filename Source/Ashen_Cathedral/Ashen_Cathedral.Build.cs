// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Ashen_Cathedral : ModuleRules
{
	public Ashen_Cathedral(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"GameplayCameras",
			"AnimGraphRuntime",
			"Niagara",
			"AIModule",
			"NavigationSystem",
			"MotionWarping",
			"Chooser",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"LevelSequence", // 보스 페이즈 전환 컷신 재생 (ULevelSequencePlayer)
			"MovieScene"     // FMovieSceneSequencePlaybackSettings / 시퀀스 플레이어 이벤트
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// 웹 디버그 대시보드 — Shipping 빌드에서는 코드가 한 바이트도 들어가지 않는다.
		// 모든 디버그 코드는 #if AC_WEB_DEBUG 로 감싼다.
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"HTTPServer",
				"Json",
				"JsonUtilities",
				"WebSocketNetworking", // 서버 소켓은 이 플러그인에만 있다 (WebSockets 모듈은 클라이언트 전용)
				"Sockets",
			});
			PublicDefinitions.Add("AC_WEB_DEBUG=1");
		}
		else
		{
			PublicDefinitions.Add("AC_WEB_DEBUG=0");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}