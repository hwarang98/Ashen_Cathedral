// 한 Run 동안만 유지되는 상태(스테이지 진행·보상 카드·미정산 보상)를 레벨 전환 너머로 보관하는 GameInstanceSubsystem


#include "Subsystems/ACRunStateSubsystem.h"
#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
#include "DataAssets/Run/ACDataAsset_RunDefinition.h"
#include "DataAssets/Run/ACDataAsset_StageDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/SoftObjectPath.h"
#include "Kismet/GameplayStatics.h"

UACRunStateSubsystem* UACRunStateSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<UACRunStateSubsystem>() : nullptr;
}

bool UACRunStateSubsystem::IsSameLevel(const UWorld* World, const TSoftObjectPtr<UWorld>& Target)
{
	if (!World || Target.IsNull())
	{
		return false;
	}

	// FixupForPIE는 아직 로드되지 않은 레벨에 동작하지 않으므로(대상 패키지가 PIE 등록 목록에 없다),
	// 반대 방향으로 현재 월드의 UEDPIE_ 프리픽스를 벗겨 비교한다. 패키지 명 전체를 비교하므로
	// 폴더가 다른 동명 맵이 서로 일치로 판정되지 않는다.
	const FString CurrentPackage = UWorld::RemovePIEPrefix(FSoftObjectPath(World).GetLongPackageFName().ToString());
	const FString TargetPackage = Target.ToSoftObjectPath().GetLongPackageFName().ToString();

	return CurrentPackage.Equals(TargetPackage, ESearchCase::IgnoreCase);
}

bool UACRunStateSubsystem::ValidateRunDefinition(const UACDataAsset_RunDefinition* RunDefinition, TArray<TObjectPtr<UACDataAsset_StageDefinition>>& OutStages) const
{
	OutStages.Reset();

	if (!RunDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("[ACRunStateSubsystem] RunDefinition이 없어 런을 시작할 수 없습니다."));
		return false;
	}

	if (RunDefinition->OrderedStages.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ACRunStateSubsystem] RunDefinition '%s'의 OrderedStages가 비어 있어 런을 시작할 수 없습니다."), *RunDefinition->RunID.ToString());
		return false;
	}

	// 비어 있는 스테이지를 걸러내고 진행하면 디자이너가 실수를 눈치채지 못한 채 런이 짧아지므로,
	// 하나라도 문제가 있으면 전체를 실패 처리한다.
	TSet<FName> SeenStageIDs;
	for (int32 Index = 0; Index < RunDefinition->OrderedStages.Num(); ++Index)
	{
		UACDataAsset_StageDefinition* Stage = RunDefinition->OrderedStages[Index];
		if (!Stage)
		{
			UE_LOG(LogTemp, Error, TEXT("[ACRunStateSubsystem] RunDefinition '%s'의 %d번째 스테이지가 비어 있습니다."), *RunDefinition->RunID.ToString(), Index);
			OutStages.Reset();
			return false;
		}

		if (Stage->LevelAsset.IsNull())
		{
			UE_LOG(LogTemp, Error, TEXT("[ACRunStateSubsystem] 스테이지 '%s'에 LevelAsset이 지정되지 않았습니다."), *Stage->StageID.ToString());
			OutStages.Reset();
			return false;
		}

		bool bAlreadySeen = false;
		SeenStageIDs.Add(Stage->StageID, &bAlreadySeen);
		if (bAlreadySeen)
		{
			UE_LOG(LogTemp, Error, TEXT("[ACRunStateSubsystem] StageID '%s'가 RunDefinition '%s' 안에서 중복됩니다."), *Stage->StageID.ToString(), *RunDefinition->RunID.ToString());
			OutStages.Reset();
			return false;
		}

		OutStages.Add(Stage);
	}

	// 로비 레벨은 GameMode의 FallbackLobbyLevel이 대신할 수 있으므로 실패로 다루지 않는다
	if (RunDefinition->LobbyLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] RunDefinition '%s'에 LobbyLevel이 없어 GameMode의 FallbackLobbyLevel을 사용하게 됩니다."), *RunDefinition->RunID.ToString());
	}

	// 이어질 스테이지가 없는데 자동 진행을 요구하는 조합 — GetEffectiveExitPolicy가 런타임에 로비 복귀로 폴백한다
	if (const UACDataAsset_StageDefinition* LastStage = OutStages.Last())
	{
		if (LastStage->ExitPolicy == EACStageExitPolicy::AutoNextStage)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] 마지막 스테이지 '%s'에 AutoNextStage가 설정되어 있습니다. 로비 복귀로 폴백합니다."), *LastStage->StageID.ToString());
		}
	}

	return true;
}

void UACRunStateSubsystem::CommitRun(UACDataAsset_RunDefinition* RunDefinition, TArray<TObjectPtr<UACDataAsset_StageDefinition>>&& Stages)
{
	ResetRunState();

	ActiveRunDefinition = RunDefinition;
	RuntimeStageOrder = MoveTemp(Stages);
	CurrentStageIndex = 0;
	bCurrentStageCleared = false;
	bRunActive = true;
}

bool UACRunStateSubsystem::BeginRunWithDefinition(UACDataAsset_RunDefinition* RunDefinition)
{
	// 검증이 끝나기 전에는 기존 런 상태를 건드리지 않는다 — 실패한 시작 요청이 진행 중인 런을 망가뜨리면 안 된다
	TArray<TObjectPtr<UACDataAsset_StageDefinition>> ValidatedStages;
	if (!ValidateRunDefinition(RunDefinition, ValidatedStages))
	{
		return false;
	}

	CommitRun(RunDefinition, MoveTemp(ValidatedStages));
	return true;
}

bool UACRunStateSubsystem::BeginDebugRunAtLevel(UACDataAsset_RunDefinition* RunDefinition, const UWorld* CurrentWorld)
{
	TArray<TObjectPtr<UACDataAsset_StageDefinition>> ValidatedStages;
	if (!ValidateRunDefinition(RunDefinition, ValidatedStages))
	{
		return false;
	}

	// 커밋 전에 시작 인덱스를 확정한다. 못 찾으면 아무것도 바꾸지 않는다
	int32 MatchedIndex = INDEX_NONE;
	for (int32 Index = 0; Index < ValidatedStages.Num(); ++Index)
	{
		if (IsSameLevel(CurrentWorld, ValidatedStages[Index]->LevelAsset))
		{
			MatchedIndex = Index;
			break;
		}
	}

	if (MatchedIndex == INDEX_NONE)
	{
		// 인덱스 0으로 폴백하면 엉뚱한 스테이지의 보상 배수와 출구 정책이 적용되므로 시작하지 않는다
		UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] 현재 레벨 '%s'에 해당하는 스테이지가 RunDefinition '%s'에 없어 디버그 런을 시작하지 않았습니다."),
			CurrentWorld ? *CurrentWorld->GetMapName() : TEXT("None"),
			RunDefinition ? *RunDefinition->RunID.ToString() : TEXT("None"));
		return false;
	}

	// CommitRun이 배열을 가져가므로 로그에 쓸 이름은 미리 확보해 둔다
	const FName MatchedStageID = ValidatedStages[MatchedIndex]->StageID;

	CommitRun(RunDefinition, MoveTemp(ValidatedStages));
	CurrentStageIndex = MatchedIndex;
	bDebugRun = true;

	UE_LOG(LogTemp, Log, TEXT("[ACRunStateSubsystem] 디버그 런을 시작합니다 — 스테이지 %d ('%s'). 영구 데이터는 저장되지 않습니다."), MatchedIndex, *MatchedStageID.ToString());

	return true;
}

bool UACRunStateSubsystem::RestartCurrentStage()
{
	if (!bRunActive)
	{
		return false;
	}

	// 구성·인덱스·디버그 여부는 유지하고 이번 런에서 쌓은 것만 비운다
	ResetRunProgress();
	return true;
}

void UACRunStateSubsystem::SettleAndEndRun()
{
	if (bDebugRun)
	{
		// 디버그 런은 실제 플레이가 아니므로 SaveGame에 아무것도 남기지 않는다
		UE_LOG(LogTemp, Log, TEXT("[ACRunStateSubsystem] 디버그 런이므로 정산을 건너뜁니다 — 재화와 첫 클리어 기록이 저장되지 않습니다."));
		ResetRunState();
		return;
	}

	// 같은 GameInstance 에 붙어 있으므로 월드를 거치지 않고 형제 서브시스템을 직접 집는다
	if (UACMetaProgressionSubsystem* MetaProgression = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACMetaProgressionSubsystem>() : nullptr)
	{
		// 재화보다 클리어 기록을 먼저 확정한다 — 순서가 뒤바뀌면 저장 도중 중단됐을 때 보상만 받고 첫 클리어가 남는다
		for (const FGameplayTag& BossID : PendingClearedBossTags)
		{
			MetaProgression->MarkBossCleared(BossID);
		}

		for (const TPair<FGameplayTag, int32>& Pair : PendingCurrencies)
		{
			MetaProgression->AddCurrency(Pair.Key, Pair.Value);
		}
	}

	ResetRunState();
}

void UACRunStateSubsystem::AbandonRun()
{
	// 적립분을 확정하지 않고 그대로 버린다 — 사망의 대가
	ResetRunState();
}

const UACDataAsset_StageDefinition* UACRunStateSubsystem::GetCurrentStage() const
{
	if (!bRunActive || !RuntimeStageOrder.IsValidIndex(CurrentStageIndex))
	{
		return nullptr;
	}

	return RuntimeStageOrder[CurrentStageIndex];
}

const UACDataAsset_StageDefinition* UACRunStateSubsystem::GetNextStage() const
{
	if (!bRunActive || !RuntimeStageOrder.IsValidIndex(CurrentStageIndex + 1))
	{
		return nullptr;
	}

	return RuntimeStageOrder[CurrentStageIndex + 1];
}

bool UACRunStateSubsystem::HasNextStage() const
{
	return GetNextStage() != nullptr;
}

bool UACRunStateSubsystem::IsCurrentStageFinal() const
{
	// 스테이지가 없을 때 true를 반환하면 보상 카드 등록이 막히므로(기존 IsFinalBossPending의 함정) false로 둔다
	if (!GetCurrentStage())
	{
		return false;
	}

	return !HasNextStage();
}

bool UACRunStateSubsystem::IsBossValidForCurrentStage(const FGameplayTag& BossIdentityTag) const
{
	const UACDataAsset_StageDefinition* Stage = GetCurrentStage();
	if (!Stage || !Stage->ExpectedBossID.IsValid())
	{
		// 런 구성이 없거나 기대 보스를 지정하지 않았으면 검사하지 않는다 (샌드박스 전투 허용)
		return true;
	}

	return BossIdentityTag.MatchesTagExact(Stage->ExpectedBossID);
}

EACStageExitPolicy UACRunStateSubsystem::GetEffectiveExitPolicy() const
{
	const UACDataAsset_StageDefinition* Stage = GetCurrentStage();
	if (!Stage)
	{
		return EACStageExitPolicy::NormalChoice;
	}

	if (Stage->ExitPolicy == EACStageExitPolicy::AutoNextStage && IsCurrentStageFinal())
	{
		// 이어질 스테이지가 없는데 자동 진행을 시도하면 플레이어가 아레나에 갇힌다
		UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] 마지막 스테이지 '%s'의 AutoNextStage는 진행할 곳이 없어 로비 복귀로 폴백합니다."), *Stage->StageID.ToString());
		return EACStageExitPolicy::ForceReturnToLobby;
	}

	return Stage->ExitPolicy;
}

bool UACRunStateSubsystem::MarkCurrentStageCleared()
{
	// 사망 연출 완료 이벤트가 두 번 들어와도 보상과 완료 처리가 한 번만 실행되도록 막는다
	if (!bRunActive || bCurrentStageCleared)
	{
		return false;
	}

	const UACDataAsset_StageDefinition* Stage = GetCurrentStage();
	if (!Stage)
	{
		return false;
	}

	bCurrentStageCleared = true;

	if (Stage->bCountsAsRunStage)
	{
		ClearedBossCount++;
	}

	return true;
}

bool UACRunStateSubsystem::AdvanceToNextStage()
{
	if (!bRunActive || !bCurrentStageCleared)
	{
		return false;
	}

	const UACDataAsset_StageDefinition* NextStage = GetNextStage();
	if (!NextStage)
	{
		return false;
	}

	// 이동할 수 없는 레벨로 인덱스만 전진시키면 런이 복구 불가능해진다
	if (NextStage->LevelAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[ACRunStateSubsystem] 다음 스테이지 '%s'에 LevelAsset이 없어 진행하지 않았습니다."), *NextStage->StageID.ToString());
		return false;
	}

	CurrentStageIndex++;
	bCurrentStageCleared = false;
	return true;
}

int32 UACRunStateSubsystem::GetCardStack(FName CardID) const
{
	const int32* Stack = AcquiredCardStacks.Find(CardID);
	return Stack ? *Stack : 0;
}

void UACRunStateSubsystem::AddCardStack(FName CardID, bool bIsLegendary)
{
	AcquiredCardStacks.FindOrAdd(CardID)++;

	if (bIsLegendary)
	{
		bLegendaryUsed = true;
	}
}

void UACRunStateSubsystem::DepositBossReward(const UACDataAsset_BossReward* RewardData, AActor* SourceBossActor, float Multiplier)
{
	// 런이 열리지 않은 상태에서 적립하면 이후 아무 정산 경로에서나 지급되어 버린다
	if (!bRunActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] 활성 런이 없어 보스 보상을 적립하지 않았습니다."));
		return;
	}

	if (!RewardData || !RewardData->BossID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] BossRewardData가 없거나 BossID가 비어있어 보상을 적립하지 않았습니다."));
		return;
	}

	if (!RewardData->RewardCurrency.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACRunStateSubsystem] RewardCurrency가 설정되지 않아 보상을 적립하지 않았습니다."));
		return;
	}

	if (SourceBossActor)
	{
		if (DepositedBosses.Contains(SourceBossActor))
		{
			return;
		}
		DepositedBosses.Add(SourceBossActor);
	}

	const UACMetaProgressionSubsystem* MetaProgression = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACMetaProgressionSubsystem>() : nullptr;
	if (!MetaProgression)
	{
		return;
	}

	// 첫 클리어 여부는 잡은 시점 기준으로 확정한다 — 같은 Run에서 같은 보스를 두 번 잡아도 첫 클리어 금액이 중복되지 않는다
	const bool bFirstClear = !MetaProgression->IsBossCleared(RewardData->BossID) && !PendingClearedBossTags.HasTagExact(RewardData->BossID);
	const int32 BaseAmount = MetaProgression->EvaluateBossRewardAmount(RewardData, bFirstClear);
	const int32 RewardAmount = FMath::RoundToInt(BaseAmount * Multiplier);

	PendingClearedBossTags.AddTag(RewardData->BossID);

	const int32 NewPendingAmount = GetPendingCurrency(RewardData->RewardCurrency) + RewardAmount;
	PendingCurrencies.Add(RewardData->RewardCurrency, NewPendingAmount);

	OnPendingRewardChangedDelegate.Broadcast(RewardData->RewardCurrency, NewPendingAmount);
}

int32 UACRunStateSubsystem::GetPendingCurrency(FGameplayTag CurrencyTag) const
{
	const int32* Amount = PendingCurrencies.Find(CurrencyTag);
	return Amount ? *Amount : 0;
}

void UACRunStateSubsystem::ResetRunProgress()
{
	// 지갑이 비워진 사실을 UI가 알아야 하므로, 비우기 전에 담고 있던 재화 태그를 기억해 둔다
	TArray<FGameplayTag> ClearedCurrencyTags;
	PendingCurrencies.GetKeys(ClearedCurrencyTags);

	bCurrentStageCleared = false;
	ClearedBossCount = 0;
	AcquiredCardStacks.Empty();
	bLegendaryUsed = false;
	PendingCurrencies.Empty();
	PendingClearedBossTags.Reset();
	DepositedBosses.Empty();

	for (const FGameplayTag& CurrencyTag : ClearedCurrencyTags)
	{
		OnPendingRewardChangedDelegate.Broadcast(CurrencyTag, 0);
	}
}

void UACRunStateSubsystem::ResetRunState()
{
	ResetRunProgress();

	bRunActive = false;
	bDebugRun = false;
	ActiveRunDefinition = nullptr;
	RuntimeStageOrder.Empty();
	CurrentStageIndex = 0;
}
