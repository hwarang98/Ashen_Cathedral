// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "ACGameplayTags.h"
#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
#include "SaveGame/ACSaveGame_MetaProgression.h"
#include "Kismet/GameplayStatics.h"

const FString UACMetaProgressionSubsystem::SaveSlotBaseName = TEXT("MetaProgressionSave_Slot_");
const int32 UACMetaProgressionSubsystem::SaveUserIndex = 0;

void UACMetaProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadSlot(ActiveSlotIndex);
}

void UACMetaProgressionSubsystem::LoadSlot(int32 SlotIndex)
{
	ActiveSlotIndex = SlotIndex;

	const FString SlotName = BuildSaveSlotName(SlotIndex);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		SaveGameInstance = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
	}
	else
	{
		// 이전에 로드해둔 다른 슬롯의 SaveGameInstance가 남아있으면 안 되므로 명시적으로 비운다
		SaveGameInstance = nullptr;
	}

	if (!SaveGameInstance)
	{
		SaveGameInstance = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::CreateSaveGameObject(UACSaveGame_MetaProgression::StaticClass()));
	}

	BroadcastAllCurrencies();
}

void UACMetaProgressionSubsystem::DeleteSlot(int32 SlotIndex)
{
	UGameplayStatics::DeleteGameInSlot(BuildSaveSlotName(SlotIndex), SaveUserIndex);

	if (SlotIndex != ActiveSlotIndex)
	{
		return;
	}

	// 활성 슬롯을 지웠다면 메모리에 남은 값도 함께 비워야 UI와 세이브가 어긋나지 않는다
	SaveGameInstance = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::CreateSaveGameObject(UACSaveGame_MetaProgression::StaticClass()));
	GrantedThisSession.Empty();

	BroadcastAllCurrencies();
}

FString UACMetaProgressionSubsystem::BuildSaveSlotName(int32 SlotIndex)
{
	return SaveSlotBaseName + FString::FromInt(SlotIndex);
}

TArray<FGameplayTag> UACMetaProgressionSubsystem::GetAllCurrencyTags()
{
	return {
		ACGameplayTags::MetaProgression_Currency_AshSoul,
		ACGameplayTags::MetaProgression_Currency_RelicFragment,
		ACGameplayTags::MetaProgression_Currency_CathedralSigil
	};
}

void UACMetaProgressionSubsystem::GrantBossReward(const UACDataAsset_BossReward* RewardData, AActor* SourceBossActor)
{
	if (!RewardData || !RewardData->BossID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] BossRewardData가 없거나 BossID가 비어있어 보상을 지급하지 않았습니다."));
		return;
	}

	if (!RewardData->RewardCurrency.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] RewardCurrency가 설정되지 않아 보상을 지급하지 않았습니다."));
		return;
	}

	if (!SaveGameInstance)
	{
		return;
	}

	if (SourceBossActor)
	{
		if (GrantedThisSession.Contains(SourceBossActor))
		{
			return;
		}
		GrantedThisSession.Add(SourceBossActor);
	}

	const bool bFirstClear = !SaveGameInstance->ClearedBossTags.HasTagExact(RewardData->BossID);
	const int32 RewardAmount = bFirstClear ? RewardData->FirstClearRewardAmount : RewardData->RepeatClearRewardAmount;

	SaveGameInstance->ClearedBossTags.AddTag(RewardData->BossID);
	AddCurrency(RewardData->RewardCurrency, RewardAmount);
}

int32 UACMetaProgressionSubsystem::GetCurrencyAmount(FGameplayTag CurrencyTag) const
{
	if (!SaveGameInstance)
	{
		return 0;
	}

	const int32* FoundAmount = SaveGameInstance->CurrencyAmounts.Find(CurrencyTag);
	return FoundAmount ? *FoundAmount : 0;
}

void UACMetaProgressionSubsystem::AddCurrency(FGameplayTag CurrencyTag, int32 Amount)
{
	if (!SaveGameInstance || !CurrencyTag.IsValid())
	{
		return;
	}

	const int32 NewAmount = FMath::Max(0, GetCurrencyAmount(CurrencyTag) + Amount);
	SaveGameInstance->CurrencyAmounts.Add(CurrencyTag, NewAmount);

	SaveProgress();

	OnCurrencyChangedDelegate.Broadcast(CurrencyTag, NewAmount);
}

bool UACMetaProgressionSubsystem::CanSpendCurrency(FGameplayTag CurrencyTag, int32 Amount) const
{
	if (!CurrencyTag.IsValid() || Amount <= 0)
	{
		return false;
	}

	return GetCurrencyAmount(CurrencyTag) >= Amount;
}

bool UACMetaProgressionSubsystem::SpendCurrency(FGameplayTag CurrencyTag, int32 Amount)
{
	if (!CanSpendCurrency(CurrencyTag, Amount))
	{
		return false;
	}

	AddCurrency(CurrencyTag, -Amount);
	return true;
}

bool UACMetaProgressionSubsystem::IsBossCleared(FGameplayTag BossID) const
{
	return SaveGameInstance && SaveGameInstance->ClearedBossTags.HasTagExact(BossID);
}

void UACMetaProgressionSubsystem::BroadcastAllCurrencies()
{
	for (const FGameplayTag& CurrencyTag : GetAllCurrencyTags())
	{
		OnCurrencyChangedDelegate.Broadcast(CurrencyTag, GetCurrencyAmount(CurrencyTag));
	}
}

void UACMetaProgressionSubsystem::SaveProgress()
{
	if (!SaveGameInstance)
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, BuildSaveSlotName(ActiveSlotIndex), SaveUserIndex);
}
