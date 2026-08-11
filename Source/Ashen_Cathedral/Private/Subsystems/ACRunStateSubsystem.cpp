// 한 Run 동안만 유지되는 상태(보상 카드·보스 진행도·미정산 보상)를 레벨 전환 너머로 보관하는 GameInstanceSubsystem


#include "Subsystems/ACRunStateSubsystem.h"
#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UACRunStateSubsystem* UACRunStateSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<UACRunStateSubsystem>() : nullptr;
}

void UACRunStateSubsystem::BeginRun()
{
	ResetRunState();
	bRunActive = true;
}

void UACRunStateSubsystem::SettleAndEndRun()
{
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

int32 UACRunStateSubsystem::ConsumeNextBossIndex()
{
	return NextBossIndex++;
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

	ClearedBossCount++;

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

void UACRunStateSubsystem::ResetRunState()
{
	// 지갑이 비워진 사실을 UI가 알아야 하므로, 비우기 전에 담고 있던 재화 태그를 기억해 둔다
	TArray<FGameplayTag> ClearedCurrencyTags;
	PendingCurrencies.GetKeys(ClearedCurrencyTags);

	bRunActive = false;
	NextBossIndex = 0;
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
