// JSON 파싱과 리플렉션 매핑 자동화 테스트.
// 실행: Session Frontend > Automation > PostProcessPresetImporter

#include "Engine/Scene.h"
#include "Misc/AutomationTest.h"
#include "PPPresetApplier.h"
#include "PPPresetJsonReader.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPPresetJsonParseTest,
	"PostProcessPresetImporter.JsonParse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPresetJsonParseTest::RunTest(const FString& Parameters)
{
	// 형태 1: 최상위 배열
	{
		const FString Json = TEXT(R"JSON(
		[
			{ "Preset": "LP_Test_A", "Package": "/Game/Foo", "Settings": { "BloomIntensity": 0.5 } },
			{ "Preset": "LP_Test_B", "Package": "/Game/Bar", "Settings": { "VignetteIntensity": 0.6 } }
		])JSON");

		TArray<FPPPresetEntry> Presets;
		FString Error;
		TestTrue(TEXT("배열 형태 파싱"), FPPPresetJsonReader::LoadFromString(Json, Presets, Error));
		TestEqual(TEXT("프리셋 2개"), Presets.Num(), 2);
		if (Presets.Num() == 2)
		{
			TestEqual(TEXT("첫 프리셋 이름"), Presets[0].PresetName, FString(TEXT("LP_Test_A")));
			TestEqual(TEXT("패키지 경로"), Presets[1].PackagePath, FString(TEXT("/Game/Bar")));
		}
	}

	// 형태 2: Presets 래핑
	{
		const FString Json = TEXT(R"JSON(
		{ "Presets": [ { "Preset": "Wrapped", "Settings": { "BloomIntensity": 1.0 } } ] })JSON");

		TArray<FPPPresetEntry> Presets;
		FString Error;
		TestTrue(TEXT("래핑 형태 파싱"), FPPPresetJsonReader::LoadFromString(Json, Presets, Error));
		TestEqual(TEXT("프리셋 1개"), Presets.Num(), 1);
	}

	// 형태 3: 이름을 키로 쓰는 맵
	{
		const FString Json = TEXT(R"JSON(
		{ "MapStyle": { "BloomIntensity": 0.25 } })JSON");

		TArray<FPPPresetEntry> Presets;
		FString Error;
		TestTrue(TEXT("맵 형태 파싱"), FPPPresetJsonReader::LoadFromString(Json, Presets, Error));
		TestEqual(TEXT("프리셋 1개"), Presets.Num(), 1);
		if (Presets.Num() == 1)
		{
			TestEqual(TEXT("키가 이름이 된다"), Presets[0].PresetName, FString(TEXT("MapStyle")));
		}
	}

	// 깨진 JSON은 실패해야 하고 크래시하면 안 된다.
	{
		TArray<FPPPresetEntry> Presets;
		FString Error;
		TestFalse(TEXT("깨진 JSON 거부"), FPPPresetJsonReader::LoadFromString(TEXT("{ this is not json"), Presets, Error));
		TestTrue(TEXT("에러 메시지 존재"), !Error.IsEmpty());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPPresetMappingTest,
	"PostProcessPresetImporter.Mapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPresetMappingTest::RunTest(const FString& Parameters)
{
	const FString Json = TEXT(R"JSON(
	[{
		"Preset": "MappingCase",
		"Settings": {
			"bOverride_BloomIntensity": true,
			"BloomIntensity": 0.5,
			"bOverride_VignetteIntensity": true,
			"VignetteIntensity": 0.6,
			"bOverride_AutoExposureMethod": true,
			"AutoExposureMethod": "AEM_Manual",
			"bOverride_ColorSaturation": true,
			"ColorSaturation": { "X": 0.8, "Y": 0.8, "Z": 0.8, "W": 1.0 },
			"bOverride_SceneFringeIntensity": true,
			"SceneFringeIntensity": 0.3,
			"ThisPropertyDoesNotExistInAnyEngine": 123,
			"AmbientCubemapTint": { "R": 0.5, "G": 0.6, "B": 0.7, "A": 1.0 }
		}
	}])JSON");

	TArray<FPPPresetEntry> Presets;
	FString Error;
	if (!TestTrue(TEXT("매핑용 JSON 파싱"), FPPPresetJsonReader::LoadFromString(Json, Presets, Error)) || Presets.Num() != 1)
	{
		return false;
	}

	FPostProcessSettings Settings;
	FPPApplyPlan Plan;
	FPPPresetApplier::ApplyToSettings(Presets[0], Settings, Plan);

	// 스칼라 float
	TestEqual(TEXT("BloomIntensity"), Settings.BloomIntensity, 0.5f);
	TestEqual(TEXT("VignetteIntensity"), Settings.VignetteIntensity, 0.6f);
	TestEqual(TEXT("SceneFringeIntensity"), Settings.SceneFringeIntensity, 0.3f);

	// bOverride_ 비트필드
	TestTrue(TEXT("bOverride_BloomIntensity"), Settings.bOverride_BloomIntensity != 0);
	TestTrue(TEXT("bOverride_VignetteIntensity"), Settings.bOverride_VignetteIntensity != 0);

	// enum (문자열 이름)
	TestEqual(TEXT("AutoExposureMethod"), static_cast<int32>(Settings.AutoExposureMethod), static_cast<int32>(AEM_Manual));

	// FVector4
	TestEqual(TEXT("ColorSaturation.X"), Settings.ColorSaturation.X, 0.8);
	TestEqual(TEXT("ColorSaturation.W"), Settings.ColorSaturation.W, 1.0);

	// FLinearColor (R/G/B/A 표기)
	TestEqual(TEXT("AmbientCubemapTint.G"), Settings.AmbientCubemapTint.G, 0.6f);

	// 알 수 없는 속성은 크래시 없이 경고 목록으로
	TestTrue(TEXT("알 수 없는 속성 기록"),
		Plan.UnknownProperties.Contains(TEXT("ThisPropertyDoesNotExistInAnyEngine")));

	// 오버라이드 목록
	TestTrue(TEXT("AppliedOverrides 수집"), Plan.AppliedOverrides.Num() >= 4);

	// JSON에 없는 속성은 건드리지 않는다.
	FPostProcessSettings Defaults;
	TestEqual(TEXT("JSON에 없는 값은 유지"), Settings.FilmGrainIntensity, Defaults.FilmGrainIntensity);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
