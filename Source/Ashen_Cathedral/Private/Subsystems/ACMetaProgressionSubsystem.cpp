// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "ACGameplayTags.h"
#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
#include "SaveGame/ACSaveGame_MetaProgression.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if AC_WEB_DEBUG
	#include "Debug/ACRunLogSubsystem.h"
#endif

const FString UACMetaProgressionSubsystem::SaveSlotBaseName = TEXT("MetaProgressionSave_Slot_");
const int32 UACMetaProgressionSubsystem::SaveUserIndex = 0;

#if WITH_EDITOR
FString UACMetaProgressionSubsystem::SlotNamePrefixOverrideForTests;
bool UACMetaProgressionSubsystem::bForceSaveFailureForTests = false;
#endif

namespace ACMetaProgressionInternal
{
	#if WITH_EDITOR
	// PIE에서 실제 세이브 파일을 건드리지 않도록 슬롯 이름에 붙이는 접두사
	static const TCHAR* PIESlotPrefix = TEXT("PIE_");

	static TAutoConsoleVariable<int32> CVarPIESandbox(
		TEXT("ac.MetaProgression.PIESandbox"),
		1,
		TEXT("1이면 PIE에서 메타 진행 세이브를 PIE_ 접두사가 붙은 별도 슬롯으로 읽고 쓴다(실제 세이브 보호). 0이면 실제 슬롯을 그대로 사용한다."),
		ECVF_Default);
	#endif

	static FString MakeSlotName(const FString& Prefix, const FString& BaseName, int32 SlotIndex)
	{
		return Prefix + BaseName + FString::FromInt(SlotIndex);
	}
}

void UACMetaProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 여기서 슬롯을 로드하지 않는다. UGT 슬롯 메뉴가 슬롯을 확정하기 전에는 어떤 슬롯 파일도 읽거나 쓰지 않아야 한다.
	// 그동안의 재화/인벤토리 변경은 메모리 전용 데이터에만 쌓이고 슬롯이 확정되면 버려진다.
	ResetToTransientSaveData();
}

void UACMetaProgressionSubsystem::Deinitialize()
{
	if (!FlushPendingSave())
	{
		// 종료 중이라 다음 기회가 없다. 그래도 아래 최종 집계에 포함되도록 보류 큐에 넣는다
		StashPendingWriteForRetry();
	}

	RetryPendingSlotWrites();

	if (PendingSlotWrites.Num() > 0)
	{
		// 여기까지 왔으면 진짜 유실이다. 조용히 끝내면 플레이어가 진행 상황을 잃은 줄도 모른다
		for (const TPair<FString, TObjectPtr<UACSaveGame_MetaProgression>>& PendingWrite : PendingSlotWrites)
		{
			UE_LOG(LogTemp, Error, TEXT("[ACMetaProgressionSubsystem] 종료 시점까지 슬롯 %s를 저장하지 못했습니다. 이 슬롯의 변경 사항이 유실됩니다."), *PendingWrite.Key);
		}

		PendingSlotWrites.Empty();
	}

	Super::Deinitialize();
}

void UACMetaProgressionSubsystem::LoadSlot(int32 SlotIndex)
{
	if (SlotIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 유효하지 않은 슬롯 번호 %d로 로드를 요청했습니다. 슬롯 미선택 상태를 유지합니다."), SlotIndex);
		return;
	}

	const FString SlotPrefix = ResolveSaveSlotPrefix();

	if (bHasActiveSlot && SlotIndex == ActiveSlotIndex && ActiveSlotPrefix == SlotPrefix)
	{
		// 레벨을 옮길 때마다 BP_ACGameModeBase의 BeginPlay가 슬롯 동기화를 다시 부른다.
		// 여기서 디스크를 다시 읽으면 이번 세션에 얻은 재화와 인벤토리가 되감기므로 아무것도 하지 않는다.
		// 접두사까지 비교하는 이유는 세션 도중 PIE 샌드박스를 끄면 같은 번호라도 다른 파일이 되기 때문이다.
		return;
	}

	// 슬롯을 바꾸기 전에 이전 슬롯의 미저장 변경을 먼저 확정한다.
	// 여기서 실패했는데 그냥 진행하면 SaveGameInstance가 교체되면서 그 변경이 사라지므로 보류 큐로 옮긴다
	if (!FlushPendingSave())
	{
		StashPendingWriteForRetry();
	}

	const FString SlotName = ACMetaProgressionInternal::MakeSlotName(SlotPrefix, SaveSlotBaseName, SlotIndex);

	SaveGameInstance = nullptr;
	bool bLoadFailed = false;
	bool bAdoptedPendingWrite = false;

	// 이 슬롯의 보류분이 있으면 디스크 내용보다 그쪽이 최신이다. 되찾아 와서 다시 저장 대상으로 삼는다
	if (TObjectPtr<UACSaveGame_MetaProgression>* PendingWrite = PendingSlotWrites.Find(SlotName))
	{
		SaveGameInstance = *PendingWrite;
		PendingSlotWrites.Remove(SlotName);
		bAdoptedPendingWrite = SaveGameInstance != nullptr;

		if (bAdoptedPendingWrite)
		{
			UE_LOG(LogTemp, Log, TEXT("[ACMetaProgressionSubsystem] 슬롯 %s의 저장 보류분을 되찾았습니다. 디스크 대신 이 데이터를 사용하고 다시 저장을 시도합니다."), *SlotName);
		}
	}

	if (!SaveGameInstance)
	{
		const bool bSlotFileExists = UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex);

		if (bSlotFileExists)
		{
			SaveGameInstance = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
		}

		// 파일은 있는데 읽지 못했다면 손상되었거나 다른 클래스로 저장된 것이다.
		// 빈 데이터로 덮어써 버리면 복구할 수 없으므로 저장을 막아 둔다
		bLoadFailed = bSlotFileExists && !SaveGameInstance;

		if (!SaveGameInstance)
		{
			SaveGameInstance = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::CreateSaveGameObject(UACSaveGame_MetaProgression::StaticClass()));

			// 새로 만든 빈 데이터는 옛 버전 파일이 아니다. CDO 기본값(1)을 그대로 두면
			// 바로 아래 MigrateIfNeeded()가 마이그레이션으로 오인해 빈 신규 슬롯이 dirty로 표시된다.
			SaveGameInstance->SaveVersion = UACSaveGame_MetaProgression::CurrentSaveVersion;
		}
	}

	ActiveSlotIndex = SlotIndex;
	ActiveSlotPrefix = SlotPrefix;
	bHasActiveSlot = true;
	bWarnedSaveWithoutSlot = false;
	bWarnedSaveBlocked = false;
	GrantedThisSession.Empty();

	// 옛 버전 파일이면 구조만 끌어올리고 즉시 쓰지는 않는다. 실제 변경이 생기거나 종료할 때 함께 기록된다.
	// 보류분을 되찾아 온 경우에는 아직 디스크에 없는 데이터이므로 반드시 dirty로 남긴다
	bPendingSave = SaveGameInstance->MigrateIfNeeded() || bAdoptedPendingWrite;

	// 이 빌드보다 새 버전 파일은 덮어쓰면 모르는 필드가 통째로 날아간다. 읽기만 허용한다
	const bool bFutureVersion = SaveGameInstance->SaveVersion > UACSaveGame_MetaProgression::CurrentSaveVersion;
	bSaveBlocked = bLoadFailed || bFutureVersion;

	if (bLoadFailed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 슬롯 파일 %s를 읽지 못했습니다. 원본을 보존하기 위해 이 슬롯에는 저장하지 않습니다."), *SlotName);
	}

	if (bSaveBlocked)
	{
		bPendingSave = false;
	}

	UE_LOG(LogTemp, Log, TEXT("[ACMetaProgressionSubsystem] 슬롯 %d를 활성화했습니다 (파일 %s, 인벤토리 %d항목)."), SlotIndex, *SlotName, SaveGameInstance->InventoryEntries.Num());

	BroadcastAllCurrencies();
	OnActiveSlotChangedDelegate.Broadcast(ActiveSlotIndex);
}

void UACMetaProgressionSubsystem::DeleteSlot(int32 SlotIndex)
{
	if (SlotIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 유효하지 않은 슬롯 번호 %d로 삭제를 요청했습니다."), SlotIndex);
		return;
	}

	// 활성 슬롯은 로드 시점에 고정한 접두사를 쓴다. 세션 도중 PIE 샌드박스 설정이 바뀌었다고 해서
	// 지금 로드해 둔 것과 다른 파일을 지워서는 안 된다. 비활성 슬롯은 현재 규칙으로 이름을 만든다.
	const bool bIsActiveSlot = bHasActiveSlot && SlotIndex == ActiveSlotIndex;
	const FString SlotName = bIsActiveSlot
		? ACMetaProgressionInternal::MakeSlotName(ActiveSlotPrefix, SaveSlotBaseName, SlotIndex)
		: BuildSaveSlotName(SlotIndex);

	// 애초에 파일이 없던 신규 슬롯은 삭제 실패가 아니라 이미 빈 슬롯이다
	const bool bSlotFileExists = UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex);

	if (bSlotFileExists && !UGameplayStatics::DeleteGameInSlot(SlotName, SaveUserIndex))
	{
		// 파일이 남아 있는데 메모리만 비우면, 다음 로드에서 지웠다고 생각한 데이터가 되살아난다.
		// 삭제가 실제로 성공할 때까지 아무것도 바꾸지 않는다.
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 슬롯 파일 %s 삭제에 실패했습니다. 파일이 남아 있으므로 메모리 상태를 그대로 유지합니다."), *SlotName);
		return;
	}

	// 지운 슬롯의 보류분이 남아 있으면 다음 재시도가 방금 지운 슬롯을 되살린다
	PendingSlotWrites.Remove(SlotName);

	if (!bIsActiveSlot)
	{
		return;
	}

	// 활성 슬롯을 지웠다면 메모리에 남은 값도 함께 비우고 슬롯 미선택 상태로 되돌린다.
	// 활성 슬롯을 유지한 채 dirty 플래그만 지우는 방식은, 플래그를 지우는 걸 한 번만 빠뜨려도
	// 다음 flush가 방금 지운 슬롯을 되살린다. 슬롯 자체를 놓아버리면 되살릴 경로가 구조적으로 사라진다.
	// UGT의 새 게임 흐름(삭제 -> OpenLevel -> SyncMetaProgressionSlot)은 곧바로 LoadSlot을 다시 부르므로 영향이 없다.
	ResetToTransientSaveData();

	BroadcastAllCurrencies();
	OnActiveSlotChangedDelegate.Broadcast(ActiveSlotIndex);
}

FString UACMetaProgressionSubsystem::BuildSaveSlotName(int32 SlotIndex) const
{
	return ACMetaProgressionInternal::MakeSlotName(ResolveSaveSlotPrefix(), SaveSlotBaseName, SlotIndex);
}

FString UACMetaProgressionSubsystem::ResolveSaveSlotPrefix() const
{
	#if WITH_EDITOR
	if (!SlotNamePrefixOverrideForTests.IsEmpty())
	{
		return SlotNamePrefixOverrideForTests;
	}
	#endif

	#if WITH_EDITOR
	const UGameInstance* OwningGameInstance = GetGameInstance();
	const UWorld* World = OwningGameInstance ? OwningGameInstance->GetWorld() : nullptr;
	if (World && World->WorldType == EWorldType::PIE && ACMetaProgressionInternal::CVarPIESandbox.GetValueOnGameThread() != 0)
	{
		return ACMetaProgressionInternal::PIESlotPrefix;
	}
	#endif

	return FString();
}

#if WITH_EDITOR
void UACMetaProgressionSubsystem::SetSlotNamePrefixOverrideForTests(const FString& InPrefix)
{
	SlotNamePrefixOverrideForTests = InPrefix;
}

void UACMetaProgressionSubsystem::InitializeForTests()
{
	ResetToTransientSaveData();
}

void UACMetaProgressionSubsystem::SetForceSaveFailureForTests(bool bInForceFailure)
{
	bForceSaveFailureForTests = bInForceFailure;
}
#endif

void UACMetaProgressionSubsystem::ResetToTransientSaveData()
{
	SaveGameInstance = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::CreateSaveGameObject(UACSaveGame_MetaProgression::StaticClass()));
	SaveGameInstance->SaveVersion = UACSaveGame_MetaProgression::CurrentSaveVersion;

	ActiveSlotIndex = INDEX_NONE;
	ActiveSlotPrefix.Reset();
	bHasActiveSlot = false;
	bPendingSave = false;
	bWarnedSaveWithoutSlot = false;
	bSaveBlocked = false;
	bWarnedSaveBlocked = false;
	GrantedThisSession.Empty();
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

int32 UACMetaProgressionSubsystem::EvaluateBossRewardAmount(const UACDataAsset_BossReward* RewardData, bool bFirstClear) const
{
	if (!RewardData)
	{
		return 0;
	}

	return bFirstClear ? RewardData->FirstClearRewardAmount : RewardData->RepeatClearRewardAmount;
}

void UACMetaProgressionSubsystem::MarkBossCleared(FGameplayTag BossID)
{
	if (!SaveGameInstance || !BossID.IsValid() || SaveGameInstance->ClearedBossTags.HasTagExact(BossID))
	{
		return;
	}

	SaveGameInstance->ClearedBossTags.AddTag(BossID);
	MarkDirtyAndSave();
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

	MarkDirtyAndSave();

#if AC_WEB_DEBUG
	// 런 로그의 currencyEarned — 획득분만 누적한다(소비는 제외)
	if (Amount > 0)
	{
		// 같은 GameInstance 에 붙어 있으므로 월드를 거치지 않고 형제 서브시스템을 직접 집는다
		if (UACRunLogSubsystem* RunLog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACRunLogSubsystem>() : nullptr)
		{
			RunLog->NotifyCurrencyEarned(CurrencyTag, Amount);
		}
	}
#endif

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

const TArray<FACSavedInventoryEntry>& UACMetaProgressionSubsystem::GetSavedInventoryEntries() const
{
	if (!SaveGameInstance)
	{
		static const TArray<FACSavedInventoryEntry> EmptyEntries;
		return EmptyEntries;
	}

	return SaveGameInstance->InventoryEntries;
}

void UACMetaProgressionSubsystem::SetSavedInventoryEntries(TArray<FACSavedInventoryEntry>&& InEntries)
{
	if (!SaveGameInstance)
	{
		return;
	}

	SaveGameInstance->InventoryEntries = MoveTemp(InEntries);
}

FPrimaryAssetId UACMetaProgressionSubsystem::GetEquippedItemDefinitionId() const
{
	return SaveGameInstance ? SaveGameInstance->EquippedItemDefinitionId : FPrimaryAssetId();
}

bool UACMetaProgressionSubsystem::SetEquippedItemDefinitionId(const FPrimaryAssetId& InItemDefinitionId)
{
	if (!SaveGameInstance || SaveGameInstance->EquippedItemDefinitionId == InItemDefinitionId)
	{
		return false;
	}

	SaveGameInstance->EquippedItemDefinitionId = InItemDefinitionId;
	return true;
}

bool UACMetaProgressionSubsystem::RequestSave()
{
	bPendingSave = true;
	return SaveProgress();
}

void UACMetaProgressionSubsystem::MarkDirtyAndSave()
{
	bPendingSave = true;
	SaveProgress();
}

void UACMetaProgressionSubsystem::BroadcastAllCurrencies()
{
	for (const FGameplayTag& CurrencyTag : GetAllCurrencyTags())
	{
		OnCurrencyChangedDelegate.Broadcast(CurrencyTag, GetCurrencyAmount(CurrencyTag));
	}
}

bool UACMetaProgressionSubsystem::FlushPendingSave()
{
	if (!bPendingSave || !bHasActiveSlot)
	{
		// 쓸 것이 없으면 실패가 아니다
		return true;
	}

	return SaveProgress();
}

void UACMetaProgressionSubsystem::StashPendingWriteForRetry()
{
	// 저장이 막힌 슬롯(손상·미래 버전)은 애초에 쓰면 안 되므로 보류하지도 않는다
	if (!SaveGameInstance || !bHasActiveSlot || bSaveBlocked)
	{
		return;
	}

	const FString SlotName = ACMetaProgressionInternal::MakeSlotName(ActiveSlotPrefix, SaveSlotBaseName, ActiveSlotIndex);

	// 같은 슬롯의 이전 보류분은 지금 것이 더 최신이므로 덮어쓴다
	PendingSlotWrites.Add(SlotName, SaveGameInstance);
	bPendingSave = false;

	UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 슬롯 %s에 쓰지 못한 채 슬롯을 전환합니다. 데이터를 보류 큐에 담아 두고 이후 저장 시점마다 다시 시도합니다 (보류 %d건)."), *SlotName, PendingSlotWrites.Num());
}

void UACMetaProgressionSubsystem::RetryPendingSlotWrites()
{
	if (PendingSlotWrites.IsEmpty())
	{
		return;
	}

	TArray<FString> WrittenSlotNames;

	for (const TPair<FString, TObjectPtr<UACSaveGame_MetaProgression>>& PendingWrite : PendingSlotWrites)
	{
		if (!PendingWrite.Value)
		{
			WrittenSlotNames.Add(PendingWrite.Key);
			continue;
		}

		if (WriteSaveGameToSlot(PendingWrite.Value, PendingWrite.Key))
		{
			WrittenSlotNames.Add(PendingWrite.Key);
			UE_LOG(LogTemp, Log, TEXT("[ACMetaProgressionSubsystem] 보류해 둔 슬롯 %s 저장에 성공했습니다."), *PendingWrite.Key);
		}
	}

	for (const FString& WrittenSlotName : WrittenSlotNames)
	{
		PendingSlotWrites.Remove(WrittenSlotName);
	}
}

bool UACMetaProgressionSubsystem::WriteSaveGameToSlot(UACSaveGame_MetaProgression* InSaveGame, const FString& InSlotName) const
{
	if (!InSaveGame)
	{
		return false;
	}

	#if WITH_EDITOR
	if (bForceSaveFailureForTests)
	{
		// 저장 실패 경로 테스트용 — 실제 쓰기를 하지 않고 실패로 처리한다
		return false;
	}
	#endif

	return UGameplayStatics::SaveGameToSlot(InSaveGame, InSlotName, SaveUserIndex);
}

bool UACMetaProgressionSubsystem::SaveProgress()
{
	if (!SaveGameInstance)
	{
		return false;
	}

	if (!bHasActiveSlot)
	{
		// 슬롯이 확정되기 전의 변경은 메모리에만 남긴다. 슬롯 0으로 폴백하면 다른 세이브를 덮어쓴다.
		// dirty 플래그는 내리지 않지만, FlushPendingSave가 활성 슬롯이 없으면 아무것도 쓰지 않으므로
		// 이 변경이 나중에 엉뚱한 슬롯으로 새어 들어가지는 않는다 (LoadSlot이 플래그를 다시 세팅한다).
		if (!bWarnedSaveWithoutSlot)
		{
			bWarnedSaveWithoutSlot = true;
			UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 활성 슬롯이 없어 저장을 건너뜁니다. UGT 슬롯 선택 전의 변경은 메모리에만 남고 슬롯을 로드하면 버려집니다."));
		}
		return false;
	}

	if (bSaveBlocked)
	{
		if (!bWarnedSaveBlocked)
		{
			bWarnedSaveBlocked = true;
			UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 이 슬롯은 저장이 막혀 있습니다(손상된 파일이거나 이 빌드보다 새 버전). 변경은 메모리에만 남습니다."));
		}
		return false;
	}

	// 디스크가 다시 살아났다면 밀려 있던 다른 슬롯부터 정리한다
	RetryPendingSlotWrites();

	SaveGameInstance->SaveVersion = UACSaveGame_MetaProgression::CurrentSaveVersion;

	const FString SlotName = ACMetaProgressionInternal::MakeSlotName(ActiveSlotPrefix, SaveSlotBaseName, ActiveSlotIndex);

	if (!WriteSaveGameToSlot(SaveGameInstance, SlotName))
	{
		// dirty를 유지해야 슬롯 전환·종료 시 FlushPendingSave가 다시 시도한다.
		// 여기서 플래그를 내리면 이번 변경(재화/보스 기록/인벤토리/장착 ID)이 조용히 사라진다.
		// UGameplayStatics는 실패 사유를 돌려주지 않으므로 판단에 필요한 맥락을 대신 남긴다.
		UE_LOG(LogTemp, Warning, TEXT("[ACMetaProgressionSubsystem] 슬롯 저장에 실패했습니다 (슬롯 %s, UserIndex %d, 인벤토리 %d항목). 디스크 공간·권한·파일 잠금을 확인하세요. 변경은 미저장 상태로 남으며 슬롯 전환이나 종료 시 다시 시도합니다."),
			*SlotName, SaveUserIndex, SaveGameInstance->InventoryEntries.Num());
		return false;
	}

	bPendingSave = false;
	return true;
}
