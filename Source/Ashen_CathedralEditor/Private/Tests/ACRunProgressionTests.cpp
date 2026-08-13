// UACRunStateSubsystem의 스테이지 진행 규칙을 월드 없이 검증한다 — 인덱스가 언제 움직이고 언제 움직이면 안 되는지가 핵심

#include "Misc/AutomationTest.h"

#include "DataAssets/Run/ACDataAsset_RunDefinition.h"
#include "DataAssets/Run/ACDataAsset_StageDefinition.h"
#include "Engine/GameInstance.h"
#include "Subsystems/ACRunStateSubsystem.h"

#if WITH_AUTOMATION_TESTS

namespace ACRunProgressionTests
{
	/**
	 * 서브시스템은 UCLASS(Within = GameInstance)라 GameInstance 아우터가 필요하다.
	 * GameInstance::Init()은 부르지 않는다 — 다른 서브시스템과 SaveGame이 함께 깨어나 부작용을 남기기 때문이다.
	 * 따라서 이 테스트들은 MetaProgression에 접근하지 않는 진행 규칙만 다룬다.
	 */
	struct FRunStateFixture
	{
		UGameInstance* GameInstance = nullptr;
		UACRunStateSubsystem* RunState = nullptr;
		TArray<UACDataAsset_StageDefinition*> Stages;

		FRunStateFixture()
		{
			GameInstance = NewObject<UGameInstance>(GetTransientPackage());
			RunState = NewObject<UACRunStateSubsystem>(GameInstance);

			GameInstance->AddToRoot();
			RunState->AddToRoot();
		}

		~FRunStateFixture()
		{
			for (UACDataAsset_StageDefinition* Stage : Stages)
			{
				if (Stage)
				{
					Stage->RemoveFromRoot();
				}
			}
			RunState->RemoveFromRoot();
			GameInstance->RemoveFromRoot();
		}

		UACDataAsset_StageDefinition* MakeStage(const FName StageID, const bool bValidLevel = true)
		{
			UACDataAsset_StageDefinition* Stage = NewObject<UACDataAsset_StageDefinition>(GetTransientPackage());
			Stage->AddToRoot();
			Stage->StageID = StageID;
			if (bValidLevel)
			{
				// 실제로 로드하지 않으므로 경로만 있으면 IsNull() 판정에 충분하다
				Stage->LevelAsset = TSoftObjectPtr<UWorld>(FSoftObjectPath(FString::Printf(TEXT("/Game/Tests/L_%s.L_%s"), *StageID.ToString(), *StageID.ToString())));
			}
			Stages.Add(Stage);
			return Stage;
		}

		UACDataAsset_RunDefinition* MakeRun(const TArray<UACDataAsset_StageDefinition*>& InStages)
		{
			UACDataAsset_RunDefinition* RunDefinition = NewObject<UACDataAsset_RunDefinition>(GameInstance);
			RunDefinition->RunID = TEXT("TestRun");
			RunDefinition->LobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Tests/L_Lobby.L_Lobby")));
			for (UACDataAsset_StageDefinition* Stage : InStages)
			{
				RunDefinition->OrderedStages.Add(Stage);
			}
			return RunDefinition;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunEmptyDefinitionRejectedTest,
	"AshenCathedral.Run.EmptyDefinitionRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunEmptyDefinitionRejectedTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	// 거부 사유를 로그로 남기는 것이 의도된 동작이므로 테스트 실패로 집계되지 않게 선언한다
	AddExpectedError(TEXT("RunDefinition이 없어 런을 시작할 수 없습니다"), EAutomationExpectedErrorFlags::Contains, 0);
	AddExpectedError(TEXT("OrderedStages가 비어 있어"), EAutomationExpectedErrorFlags::Contains, 0);

	TestFalse(TEXT("null RunDefinition은 거부된다"), Fixture.RunState->BeginRunWithDefinition(nullptr));
	TestFalse(TEXT("거부 후에도 런이 열리지 않는다"), Fixture.RunState->IsRunActive());

	UACDataAsset_RunDefinition* EmptyRun = Fixture.MakeRun({});
	TestFalse(TEXT("OrderedStages가 비면 거부된다"), Fixture.RunState->BeginRunWithDefinition(EmptyRun));
	TestFalse(TEXT("거부 후에도 런이 열리지 않는다"), Fixture.RunState->IsRunActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunNullStageAbortsWholeRunTest,
	"AshenCathedral.Run.NullStageAbortsWholeRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunNullStageAbortsWholeRunTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	// 비어 있는 스테이지를 걸러내고 진행하면 디자이너가 실수를 눈치채지 못하므로 전체 실패여야 한다
	UACDataAsset_RunDefinition* RunDefinition = Fixture.MakeRun({ Fixture.MakeStage(TEXT("A")), nullptr, Fixture.MakeStage(TEXT("C")) });

	AddExpectedError(TEXT("스테이지가 비어 있습니다"), EAutomationExpectedErrorFlags::Contains, 0);
	TestFalse(TEXT("null 스테이지가 있으면 런 전체가 실패한다"), Fixture.RunState->BeginRunWithDefinition(RunDefinition));
	TestFalse(TEXT("런이 열리지 않는다"), Fixture.RunState->IsRunActive());

	// LevelAsset이 없는 스테이지도 마찬가지다
	UACDataAsset_RunDefinition* NoLevelRun = Fixture.MakeRun({ Fixture.MakeStage(TEXT("D")), Fixture.MakeStage(TEXT("E"), /*bValidLevel*/ false) });

	AddExpectedError(TEXT("LevelAsset이 지정되지 않았습니다"), EAutomationExpectedErrorFlags::Contains, 0);
	TestFalse(TEXT("LevelAsset이 없는 스테이지가 있으면 런 전체가 실패한다"), Fixture.RunState->BeginRunWithDefinition(NoLevelRun));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunFailedBeginPreservesActiveRunTest,
	"AshenCathedral.Run.FailedBeginPreservesActiveRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunFailedBeginPreservesActiveRunTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	TestTrue(TEXT("정상 런이 열린다"), Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB })));

	Fixture.RunState->AddCardStack(TEXT("Card_Test"), /*bIsLegendary*/ false);
	TestTrue(TEXT("스테이지 클리어 처리"), Fixture.RunState->MarkCurrentStageCleared());
	TestTrue(TEXT("다음 스테이지로 진행"), Fixture.RunState->AdvanceToNextStage());

	// 진행 중인 런은 실패한 시작 요청에 영향을 받으면 안 된다
	AddExpectedError(TEXT("OrderedStages가 비어 있어"), EAutomationExpectedErrorFlags::Contains, 0);
	TestFalse(TEXT("잘못된 정의는 거부된다"), Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({})));

	TestTrue(TEXT("기존 런이 그대로 살아 있다"), Fixture.RunState->IsRunActive());
	TestTrue(TEXT("현재 스테이지가 유지된다"), Fixture.RunState->GetCurrentStage() == StageB);
	TestEqual(TEXT("카드 중첩이 유지된다"), Fixture.RunState->GetCardStack(TEXT("Card_Test")), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunFirstStageIndexIsZeroTest,
	"AshenCathedral.Run.FirstStageIndexIsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunFirstStageIndexIsZeroTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	TestTrue(TEXT("런이 열린다"), Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB })));

	TestTrue(TEXT("첫 스테이지는 0번이다"), Fixture.RunState->GetCurrentStage() == StageA);
	TestTrue(TEXT("다음 스테이지는 1번이다"), Fixture.RunState->GetNextStage() == StageB);
	TestFalse(TEXT("시작 시점에는 클리어되지 않은 상태다"), Fixture.RunState->IsCurrentStageCleared());
	TestFalse(TEXT("디버그 런이 아니다"), Fixture.RunState->IsDebugRun());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunAdvanceRequiresClearedTest,
	"AshenCathedral.Run.AdvanceRequiresCleared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunAdvanceRequiresClearedTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, Fixture.MakeStage(TEXT("B")) }));

	// 레벨 로드나 보스 등록만으로는 인덱스가 움직이면 안 된다
	TestFalse(TEXT("클리어 전에는 진행할 수 없다"), Fixture.RunState->AdvanceToNextStage());
	TestTrue(TEXT("인덱스가 그대로다"), Fixture.RunState->GetCurrentStage() == StageA);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunAdvanceAfterClearedIncrementsOnceTest,
	"AshenCathedral.Run.AdvanceAfterClearedIncrementsOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunAdvanceAfterClearedIncrementsOnceTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB }));

	TestTrue(TEXT("클리어 처리된다"), Fixture.RunState->MarkCurrentStageCleared());
	TestTrue(TEXT("한 번은 진행된다"), Fixture.RunState->AdvanceToNextStage());
	TestTrue(TEXT("다음 스테이지로 이동했다"), Fixture.RunState->GetCurrentStage() == StageB);
	TestFalse(TEXT("진행 후 클리어 플래그가 리셋된다"), Fixture.RunState->IsCurrentStageCleared());

	TestFalse(TEXT("연속 호출은 다시 진행하지 않는다"), Fixture.RunState->AdvanceToNextStage());
	TestTrue(TEXT("인덱스가 더 움직이지 않았다"), Fixture.RunState->GetCurrentStage() == StageB);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunInvalidNextLevelBlocksAdvanceTest,
	"AshenCathedral.Run.InvalidNextLevelBlocksAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunInvalidNextLevelBlocksAdvanceTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB }));

	// 시작 이후에 레벨 참조가 깨진 상황 — 인덱스만 전진시키면 런이 복구 불가능해진다
	StageB->LevelAsset.Reset();

	TestTrue(TEXT("클리어 처리된다"), Fixture.RunState->MarkCurrentStageCleared());

	AddExpectedError(TEXT("LevelAsset이 없어 진행하지 않았습니다"), EAutomationExpectedErrorFlags::Contains, 0);
	TestFalse(TEXT("다음 레벨이 무효하면 진행하지 않는다"), Fixture.RunState->AdvanceToNextStage());
	TestTrue(TEXT("인덱스가 그대로다"), Fixture.RunState->GetCurrentStage() == StageA);
	TestTrue(TEXT("클리어 상태는 유지된다"), Fixture.RunState->IsCurrentStageCleared());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunMarkClearedIsIdempotentTest,
	"AshenCathedral.Run.MarkClearedIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunMarkClearedIsIdempotentTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ Fixture.MakeStage(TEXT("A")), Fixture.MakeStage(TEXT("B")) }));

	// 사망 연출 완료 이벤트가 중복으로 들어와도 보상이 두 번 적립되면 안 된다
	TestTrue(TEXT("첫 호출은 성공한다"), Fixture.RunState->MarkCurrentStageCleared());
	TestEqual(TEXT("클리어 수가 1이다"), Fixture.RunState->GetClearedBossCount(), 1);

	TestFalse(TEXT("두 번째 호출은 false다"), Fixture.RunState->MarkCurrentStageCleared());
	TestEqual(TEXT("클리어 수가 늘지 않았다"), Fixture.RunState->GetClearedBossCount(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunFinalStageDetectionTest,
	"AshenCathedral.Run.FinalStageDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunFinalStageDetectionTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB }));

	TestFalse(TEXT("첫 스테이지는 최종이 아니다"), Fixture.RunState->IsCurrentStageFinal());
	TestTrue(TEXT("다음 스테이지가 있다"), Fixture.RunState->HasNextStage());

	Fixture.RunState->MarkCurrentStageCleared();
	Fixture.RunState->AdvanceToNextStage();

	TestTrue(TEXT("마지막 스테이지는 최종이다"), Fixture.RunState->IsCurrentStageFinal());
	TestFalse(TEXT("다음 스테이지가 없다"), Fixture.RunState->HasNextStage());

	Fixture.RunState->MarkCurrentStageCleared();
	TestFalse(TEXT("클리어해도 더 진행할 수 없다"), Fixture.RunState->AdvanceToNextStage());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunFinalStageAutoNextFallsBackTest,
	"AshenCathedral.Run.FinalStageAutoNextFallsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunFinalStageAutoNextFallsBackTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	StageA->ExitPolicy = EACStageExitPolicy::AutoNextStage;

	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	StageB->ExitPolicy = EACStageExitPolicy::AutoNextStage;

	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB }));

	// 이어질 스테이지가 있으면 설정대로 자동 진행한다
	TestTrue(TEXT("비최종 스테이지는 AutoNextStage 그대로다"), Fixture.RunState->GetEffectiveExitPolicy() == EACStageExitPolicy::AutoNextStage);

	Fixture.RunState->MarkCurrentStageCleared();
	Fixture.RunState->AdvanceToNextStage();

	// 마지막 스테이지에서 자동 진행하면 나갈 곳이 없어 플레이어가 갇힌다
	TestTrue(TEXT("마지막 스테이지에서는 로비 복귀로 폴백한다"), Fixture.RunState->GetEffectiveExitPolicy() == EACStageExitPolicy::ForceReturnToLobby);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunBossIdentityMismatchBlocksTest,
	"AshenCathedral.Run.BossIdentityMismatchBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunBossIdentityMismatchBlocksTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	const FGameplayTag ExpectedTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Boss.Aldren"), /*ErrorIfNotFound*/ false);
	const FGameplayTag OtherTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Boss.Ordan"), /*ErrorIfNotFound*/ false);
	if (!ExpectedTag.IsValid() || !OtherTag.IsValid())
	{
		AddWarning(TEXT("Enemy.Boss 태그가 등록되어 있지 않아 검사를 건너뜁니다."));
		return true;
	}

	// 런 구성이 없으면 검사하지 않는다 (샌드박스 전투 허용)
	TestTrue(TEXT("활성 런이 없으면 모든 보스가 허용된다"), Fixture.RunState->IsBossValidForCurrentStage(OtherTag));

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	StageA->ExpectedBossID = ExpectedTag;
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA }));

	TestTrue(TEXT("기대하는 보스는 통과한다"), Fixture.RunState->IsBossValidForCurrentStage(ExpectedTag));
	TestFalse(TEXT("다른 보스는 거부된다"), Fixture.RunState->IsBossValidForCurrentStage(OtherTag));
	TestFalse(TEXT("빈 태그도 거부된다"), Fixture.RunState->IsBossValidForCurrentStage(FGameplayTag()));

	// ExpectedBossID를 지정하지 않았으면 검사 자체를 하지 않는다
	StageA->ExpectedBossID = FGameplayTag();
	TestTrue(TEXT("ExpectedBossID가 없으면 검사하지 않는다"), Fixture.RunState->IsBossValidForCurrentStage(OtherTag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunAbandonResetsRunStateTest,
	"AshenCathedral.Run.AbandonResetsRunState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunAbandonResetsRunStateTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ Fixture.MakeStage(TEXT("A")), Fixture.MakeStage(TEXT("B")) }));
	Fixture.RunState->AddCardStack(TEXT("Card_Test"), /*bIsLegendary*/ true);
	Fixture.RunState->MarkCurrentStageCleared();
	Fixture.RunState->AdvanceToNextStage();

	Fixture.RunState->AbandonRun();

	TestFalse(TEXT("런이 닫힌다"), Fixture.RunState->IsRunActive());
	TestTrue(TEXT("현재 스테이지가 없다"), Fixture.RunState->GetCurrentStage() == nullptr);
	TestEqual(TEXT("카드가 소실된다"), Fixture.RunState->GetCardStack(TEXT("Card_Test")), 0);
	TestFalse(TEXT("전설 사용 플래그가 초기화된다"), Fixture.RunState->IsLegendaryUsed());
	TestEqual(TEXT("클리어 수가 초기화된다"), Fixture.RunState->GetClearedBossCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunCountsAsRunStageGatesClearedCountTest,
	"AshenCathedral.Run.CountsAsRunStageGatesClearedCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunCountsAsRunStageGatesClearedCountTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* TutorialStage = Fixture.MakeStage(TEXT("Tutorial"));
	TutorialStage->bCountsAsRunStage = false;

	UACDataAsset_StageDefinition* NormalStage = Fixture.MakeStage(TEXT("Normal"));
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ TutorialStage, NormalStage }));

	TestTrue(TEXT("튜토리얼 스테이지도 클리어 처리된다"), Fixture.RunState->MarkCurrentStageCleared());
	TestEqual(TEXT("집계에서는 제외된다"), Fixture.RunState->GetClearedBossCount(), 0);

	Fixture.RunState->AdvanceToNextStage();
	TestTrue(TEXT("일반 스테이지 클리어"), Fixture.RunState->MarkCurrentStageCleared());
	TestEqual(TEXT("일반 스테이지는 집계된다"), Fixture.RunState->GetClearedBossCount(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunNoRunIsNotFinalStageTest,
	"AshenCathedral.Run.NoRunIsNotFinalStage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunNoRunIsNotFinalStageTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	// 활성 런이 없을 때 true를 반환하면 보상 카드 등록이 막힌다 (기존 IsFinalBossPending의 함정)
	TestFalse(TEXT("런이 없으면 최종 스테이지가 아니다"), Fixture.RunState->IsCurrentStageFinal());
	TestTrue(TEXT("현재 스테이지가 없다"), Fixture.RunState->GetCurrentStage() == nullptr);
	TestTrue(TEXT("출구 정책은 기본값이다"), Fixture.RunState->GetEffectiveExitPolicy() == EACStageExitPolicy::NormalChoice);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunRestartCurrentStageKeepsPositionTest,
	"AshenCathedral.Run.RestartCurrentStageKeepsPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunRestartCurrentStageKeepsPositionTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	UACDataAsset_StageDefinition* StageA = Fixture.MakeStage(TEXT("A"));
	UACDataAsset_StageDefinition* StageB = Fixture.MakeStage(TEXT("B"));
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ StageA, StageB }));

	Fixture.RunState->MarkCurrentStageCleared();
	Fixture.RunState->AdvanceToNextStage();
	Fixture.RunState->AddCardStack(TEXT("Card_Test"), /*bIsLegendary*/ false);
	Fixture.RunState->MarkCurrentStageCleared();

	TestTrue(TEXT("재시작이 처리된다"), Fixture.RunState->RestartCurrentStage());

	TestTrue(TEXT("런이 유지된다"), Fixture.RunState->IsRunActive());
	TestTrue(TEXT("스테이지 인덱스가 유지된다"), Fixture.RunState->GetCurrentStage() == StageB);
	TestFalse(TEXT("클리어 플래그는 초기화된다"), Fixture.RunState->IsCurrentStageCleared());
	TestEqual(TEXT("카드는 초기화된다"), Fixture.RunState->GetCardStack(TEXT("Card_Test")), 0);
	TestEqual(TEXT("클리어 수는 초기화된다"), Fixture.RunState->GetClearedBossCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACRunDebugRestartKeepsDebugFlagTest,
	"AshenCathedral.Run.DebugRestartKeepsDebugFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACRunDebugRestartKeepsDebugFlagTest::RunTest(const FString& Parameters)
{
	ACRunProgressionTests::FRunStateFixture Fixture;

	// BeginDebugRunAtLevel은 실제 UWorld가 필요하므로, 여기서는 재시작이 플래그를 보존하는지만 확인한다.
	// 디버그 런의 레벨 탐색 자체는 PIE 수동 검증 대상이다.
	Fixture.RunState->BeginRunWithDefinition(Fixture.MakeRun({ Fixture.MakeStage(TEXT("A")) }));
	TestFalse(TEXT("일반 런은 디버그 런이 아니다"), Fixture.RunState->IsDebugRun());

	Fixture.RunState->MarkCurrentStageCleared();
	TestTrue(TEXT("재시작이 처리된다"), Fixture.RunState->RestartCurrentStage());
	TestFalse(TEXT("재시작 후에도 디버그 플래그가 그대로다"), Fixture.RunState->IsDebugRun());
	TestTrue(TEXT("런이 유지된다"), Fixture.RunState->IsRunActive());

	// 런이 없으면 재시작할 것도 없다
	Fixture.RunState->AbandonRun();
	TestFalse(TEXT("활성 런이 없으면 재시작이 실패한다"), Fixture.RunState->RestartCurrentStage());

	return true;
}

#endif // WITH_AUTOMATION_TESTS
