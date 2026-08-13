// 워크벤치의 에셋 배선 검사기를 자동화 테스트로 노출한다 — 에디터 UI를 열지 않고 CI에서 돌리기 위한 것

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/CurveTable.h"
#include "Validation/ACAttributeAudit.h"
#include "Validation/ACBossAssetValidator.h"
#include "Validation/ACDecayPairValidator.h"
#include "Validation/ACMontageAudit.h"
#include "Validation/ACRunAssetValidator.h"
#include "Validation/ACStateTreeValidator.h"

#if WITH_AUTOMATION_TESTS

namespace ACAssetValidationTests
{
	/**
	 * 검사기는 전부 에셋 레지스트리 질의로 대상을 찾는다.
	 * 헤드리스 실행에서는 스캔이 끝나기 전에 테스트가 시작될 수 있고, 그러면 대상을 못 찾고 조용히 통과한다.
	 */
	void WaitForAssetRegistry()
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().SearchAllAssets(true);
		AssetRegistryModule.Get().WaitForCompletion();
	}

	/** 검사 결과를 테스트 결과로 옮긴다. Info는 정상 확인이거나 참고용이라 반영하지 않는다 */
	void ReportIssues(FAutomationTestBase& Test, const TArray<FACAssetReport>& Reports)
	{
		for (const FACAssetReport& Report : Reports)
		{
			for (const FACAssetIssue& Issue : Report.Issues)
			{
				const FString Message = FString::Printf(TEXT("%s — %s"), *Report.AssetName, *Issue.Message);

				if (Issue.Severity == EACAssetIssueSeverity::Error)
				{
					Test.AddError(Message);
				}
				else if (Issue.Severity == EACAssetIssueSeverity::Warning)
				{
					Test.AddWarning(Message);
				}
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACStateTreeWiringTest,
	"AshenCathedral.Assets.StateTreeWiring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACStateTreeWiringTest::RunTest(const FString& Parameters)
{
	ACAssetValidationTests::WaitForAssetRegistry();

	const TArray<FACStateTreeReport> Reports = ACStateTreeValidator::ValidateAllStateTrees();
	if (Reports.IsEmpty())
	{
		AddWarning(TEXT("/Game 아래에서 StateTree 에셋을 찾지 못했습니다."));
		return true;
	}

	for (const FACStateTreeReport& Report : Reports)
	{
		for (const FACStateTreeIssue& Issue : Report.Issues)
		{
			const FString Message = FString::Printf(TEXT("%s · %s — %s"), *Report.AssetName, *Issue.StatePath, *Issue.Message);

			if (Issue.Severity == EACStateTreeIssueSeverity::Error)
			{
				AddError(Message);
			}
			else if (Issue.Severity == EACStateTreeIssueSeverity::Warning)
			{
				AddWarning(Message);
			}

			// Info는 GE나 AnimNotify로 해결됐을 수 있어 테스트 결과에 반영하지 않는다
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACDecayPairTest,
	"AshenCathedral.Assets.DecayPair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACDecayPairTest::RunTest(const FString& Parameters)
{
	ACAssetValidationTests::WaitForAssetRegistry();

	for (const FACDecayIssue& Issue : ACDecayPairValidator::ValidateAll())
	{
		const FString Message = FString::Printf(TEXT("%s · %s — %s"), *Issue.CharacterName, *Issue.GaugeName, *Issue.Message);

		if (Issue.Severity == EACDecayIssueSeverity::Error)
		{
			AddError(Message);
		}
		else
		{
			AddWarning(Message);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACAttributeWiringTest,
	"AshenCathedral.Assets.AttributeWiring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACAttributeWiringTest::RunTest(const FString& Parameters)
{
	ACAssetValidationTests::WaitForAssetRegistry();

	const TArray<FACAttributeProfile> Profiles = ACAttributeAudit::CollectAllProfiles();
	if (Profiles.IsEmpty())
	{
		AddWarning(TEXT("/Game 아래에서 Startup DataAsset을 찾지 못했습니다."));
		return true;
	}

	// 캐릭터 하나당 스탯 CurveTable 하나가 이 프로젝트의 규칙이다.
	// 의도적으로 테이블을 나누게 되면 이 검사를 함께 조정해야 한다.
	for (const FACAttributeProfile& Profile : Profiles)
	{
		if (Profile.ReferencedCurveTables.Num() <= 1)
		{
			continue;
		}

		TArray<FString> TableNames;
		for (const TWeakObjectPtr<UCurveTable>& Table : Profile.ReferencedCurveTables)
		{
			TableNames.Add(Table.IsValid() ? Table->GetName() : TEXT("(없음)"));
		}

		AddError(FString::Printf(
			TEXT("%s의 초기화 GE가 CurveTable을 %d개 참조합니다 — %s. 다른 캐릭터의 테이블이 섞였는지 확인하세요."),
			*Profile.CharacterName,
			Profile.ReferencedCurveTables.Num(),
			*FString::Join(TableNames, TEXT(", "))));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACBossAssetTest,
	"AshenCathedral.Assets.BossAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACBossAssetTest::RunTest(const FString& Parameters)
{
	ACAssetValidationTests::WaitForAssetRegistry();

	const TArray<FACAssetReport> Reports = ACBossAssetValidator::ValidateAll();
	if (Reports.IsEmpty())
	{
		AddWarning(TEXT("/Game/Enemy 아래에서 보스로 판정되는 Blueprint를 찾지 못했습니다."));
		return true;
	}

	ACAssetValidationTests::ReportIssues(*this, Reports);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACAttackMontageTest,
	"AshenCathedral.Assets.AttackMontages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACAttackMontageTest::RunTest(const FString& Parameters)
{
	ACAssetValidationTests::WaitForAssetRegistry();

	const TArray<FACAssetReport> Reports = ACMontageAudit::AuditAll();
	if (Reports.IsEmpty())
	{
		AddWarning(TEXT("/Game/Enemy 아래에서 공격 몽타주를 찾지 못했습니다."));
		return true;
	}

	ACAssetValidationTests::ReportIssues(*this, Reports);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunDefinitionWiringTest,
	"AshenCathedral.Assets.RunDefinitionWiring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunDefinitionWiringTest::RunTest(const FString& Parameters)
{
	ACAssetValidationTests::WaitForAssetRegistry();

	const TArray<FACAssetReport> Reports = ACRunAssetValidator::ValidateAll();
	if (Reports.IsEmpty())
	{
		AddWarning(TEXT("/Game 아래에서 RunDefinition 에셋을 찾지 못했습니다."));
		return true;
	}

	ACAssetValidationTests::ReportIssues(*this, Reports);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
