// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/ACMetaProgressionSubsystem.h"
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

	OnScarFragmentsChangedDelegate.Broadcast(GetScarFragments());
}

FString UACMetaProgressionSubsystem::BuildSaveSlotName(int32 SlotIndex)
{
	return SaveSlotBaseName + FString::FromInt(SlotIndex);
}

void UACMetaProgressionSubsystem::GrantBossReward(const UACDataAsset_BossReward* RewardData, AActor* SourceBossActor)
{
	if (!RewardData || !RewardData->BossID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] BossRewardData가 없거나 BossID가 비어있어 성흔 조각을 지급하지 않았습니다."));
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
	const int32 RewardAmount = bFirstClear ? RewardData->FirstClearScarFragments : RewardData->RepeatClearScarFragments;

	SaveGameInstance->ClearedBossTags.AddTag(RewardData->BossID);
	AddScarFragments(RewardAmount);
}

void UACMetaProgressionSubsystem::AddScarFragments(int32 Amount)
{
	if (!SaveGameInstance)
	{
		return;
	}

	SaveGameInstance->ScarFragments = FMath::Max(0, SaveGameInstance->ScarFragments + Amount);
	SaveProgress();

	OnScarFragmentsChangedDelegate.Broadcast(SaveGameInstance->ScarFragments);
}

int32 UACMetaProgressionSubsystem::GetScarFragments() const
{
	return SaveGameInstance ? SaveGameInstance->ScarFragments : 0;
}

bool UACMetaProgressionSubsystem::IsBossCleared(FGameplayTag BossID) const
{
	return SaveGameInstance && SaveGameInstance->ClearedBossTags.HasTagExact(BossID);
}

void UACMetaProgressionSubsystem::SaveProgress()
{
	if (!SaveGameInstance)
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, BuildSaveSlotName(ActiveSlotIndex), SaveUserIndex);
}