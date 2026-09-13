// 슬롯에 영속되는 인벤토리를 관리하는 GameInstanceSubsystem

#include "Subsystems/ACInventorySubsystem.h"
#include "DataAssets/Items/ACItemDefinition.h"
#include "SaveGame/ACSavedInventoryEntry.h"
#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Subsystems/SubsystemCollection.h"

#if WITH_EDITOR
TFunction<UACItemDefinition*(const FPrimaryAssetId&)> UACInventorySubsystem::ItemDefinitionResolverForTests;
#endif

namespace ACInventoryInternal
{
	// 인벤토리가 가질 수 있는 Entry 수 상한. 스택되지 않는 아이템에 거대한 수량을 넘겼을 때
	// Entry를 무한히 만들며 멈추는 것을 막는 안전장치다. 실제 게임 플레이에서 닿을 수 있는 값이 아니다.
	static constexpr int32 MaxInventoryEntries = 4096;

	/**
	 * 개별 상태(강화 단계, 수치 스택)가 없는 "기본 상태" 항목인지 여부.
	 * 병합은 기본 상태끼리만 허용한다 — 강화된 스택에 일반 아이템을 얹으면
	 * 새로 넣은 수량까지 강화 상태를 공짜로 얻어가기 때문이다.
	 */
	static bool IsDefaultStateEntry(const FACInventoryEntry& Entry)
	{
		return Entry.UpgradeLevel == 0 && Entry.StatTagStacks.Num() == 0;
	}
}

void UACInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 슬롯 데이터를 소유한 쪽이 먼저 살아 있어야 로드 직후 인벤토리를 만들 수 있다
	Collection.InitializeDependency<UACMetaProgressionSubsystem>();

	Super::Initialize(Collection);

	if (UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression())
	{
		MetaProgression->OnActiveSlotChangedDelegate.AddDynamic(this, &UACInventorySubsystem::HandleActiveSlotChanged);
	}

	// 슬롯이 아직 없으면 비어 있는 메모리 전용 데이터로 시작한다
	RebuildEntriesFromSaveData();
}

void UACInventorySubsystem::Deinitialize()
{
	if (UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression())
	{
		MetaProgression->OnActiveSlotChangedDelegate.RemoveDynamic(this, &UACInventorySubsystem::HandleActiveSlotChanged);
	}

	Super::Deinitialize();
}

int32 UACInventorySubsystem::AddItem(UACItemDefinition* ItemDefinition, int32 Count)
{
	if (!ItemDefinition || Count <= 0)
	{
		return 0;
	}

	const int32 MaxStack = ItemDefinition->GetEffectiveMaxStackCount();
	int32 Remaining = Count;

	// 빈자리가 남은 기존 항목부터 채운다
	for (FACInventoryEntry& Entry : Entries)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Entry.ItemDefinition != ItemDefinition)
		{
			continue;
		}

		// 강화·수치 상태를 가진 항목에는 합치지 않는다. 합치면 새로 넣은 수량이 그 상태를 공유해 버린다
		if (!ACInventoryInternal::IsDefaultStateEntry(Entry))
		{
			continue;
		}

		const int32 FreeSpace = MaxStack - Entry.StackCount;
		if (FreeSpace <= 0)
		{
			continue;
		}

		const int32 AddedToEntry = FMath::Min(FreeSpace, Remaining);
		Entry.StackCount += AddedToEntry;
		Remaining -= AddedToEntry;
	}

	// 남은 수량은 새 항목으로 나눠 담는다
	while (Remaining > 0)
	{
		if (Entries.Num() >= ACInventoryInternal::MaxInventoryEntries)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ACInventorySubsystem] 인벤토리 항목 수 상한(%d)에 도달해 %d개를 더 넣지 못했습니다."), ACInventoryInternal::MaxInventoryEntries, Remaining);
			break;
		}

		FACInventoryEntry NewEntry;
		NewEntry.ItemDefinition = ItemDefinition;
		NewEntry.InstanceId = FGuid::NewGuid();
		NewEntry.StackCount = FMath::Min(MaxStack, Remaining);

		Remaining -= NewEntry.StackCount;
		Entries.Add(MoveTemp(NewEntry));
	}

	const int32 AddedTotal = Count - Remaining;
	if (AddedTotal > 0)
	{
		CommitInventoryChange({ ItemDefinition });
	}

	return AddedTotal;
}

int32 UACInventorySubsystem::RemoveItem(UACItemDefinition* ItemDefinition, int32 Count)
{
	if (!ItemDefinition || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;

	// 뒤쪽 항목부터 비운다 — 먼저 얻은 항목의 강화 상태를 최대한 남기기 위해서다
	for (int32 EntryIndex = Entries.Num() - 1; EntryIndex >= 0 && Remaining > 0; --EntryIndex)
	{
		FACInventoryEntry& Entry = Entries[EntryIndex];
		if (Entry.ItemDefinition != ItemDefinition)
		{
			continue;
		}

		const int32 TakenFromEntry = FMath::Min(Entry.StackCount, Remaining);
		Entry.StackCount -= TakenFromEntry;
		Remaining -= TakenFromEntry;

		if (Entry.StackCount <= 0)
		{
			Entries.RemoveAt(EntryIndex);
		}
	}

	const int32 RemovedTotal = Count - Remaining;
	if (RemovedTotal > 0)
	{
		// 마지막 하나까지 없앴다면 장착 기록도 함께 정리해야 존재하지 않는 아이템이 장착으로 남지 않는다
		ClearEquippedIfNoLongerOwned(ItemDefinition);

		CommitInventoryChange({ ItemDefinition });
	}

	return RemovedTotal;
}

bool UACInventorySubsystem::SetStackCount(FGuid InstanceId, int32 NewStackCount)
{
	const int32 EntryIndex = Entries.IndexOfByPredicate([&InstanceId](const FACInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});

	if (EntryIndex == INDEX_NONE)
	{
		return false;
	}

	UACItemDefinition* ChangedDefinition = Entries[EntryIndex].ItemDefinition;
	const int32 ClampedCount = ChangedDefinition ? ChangedDefinition->ClampStackCount(NewStackCount) : FMath::Max(0, NewStackCount);

	if (Entries[EntryIndex].StackCount == ClampedCount)
	{
		return true;
	}

	if (ClampedCount <= 0)
	{
		Entries.RemoveAt(EntryIndex);
	}
	else
	{
		Entries[EntryIndex].StackCount = ClampedCount;
	}

	// 수량을 0으로 만든 것은 RemoveItem으로 전부 제거한 것과 같은 상태여야 한다.
	// 장착 해제 통지와 저장은 아래 CommitInventoryChange 한 번에 묶여 중복되지 않는다
	ClearEquippedIfNoLongerOwned(ChangedDefinition);

	CommitInventoryChange({ ChangedDefinition });
	return true;
}

void UACInventorySubsystem::ClearEquippedIfNoLongerOwned(const UACItemDefinition* ItemDefinition)
{
	if (!ItemDefinition || GetTotalItemCount(ItemDefinition) > 0)
	{
		return;
	}

	UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	if (!MetaProgression || MetaProgression->GetEquippedItemDefinitionId() != ItemDefinition->GetPrimaryAssetId())
	{
		return;
	}

	// 값만 비우고 저장은 호출자의 커밋에 맡긴다
	if (MetaProgression->SetEquippedItemDefinitionId(FPrimaryAssetId()))
	{
		OnEquippedItemChangedDelegate.Broadcast(nullptr);
	}
}

void UACInventorySubsystem::GatherTotalsByDefinition(TMap<UACItemDefinition*, int32>& OutTotals) const
{
	OutTotals.Reset();

	for (const FACInventoryEntry& Entry : Entries)
	{
		if (Entry.ItemDefinition)
		{
			OutTotals.FindOrAdd(Entry.ItemDefinition) += Entry.StackCount;
		}
	}
}

bool UACInventorySubsystem::SetUpgradeLevel(FGuid InstanceId, int32 NewUpgradeLevel)
{
	FACInventoryEntry* FoundEntry = Entries.FindByPredicate([&InstanceId](const FACInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});

	if (!FoundEntry)
	{
		return false;
	}

	const int32 ClampedLevel = FMath::Max(0, NewUpgradeLevel);
	if (FoundEntry->UpgradeLevel == ClampedLevel)
	{
		return true;
	}

	FoundEntry->UpgradeLevel = ClampedLevel;
	CommitInventoryChange({ FoundEntry->ItemDefinition.Get() });
	return true;
}

bool UACInventorySubsystem::AddStatTagStack(FGuid InstanceId, FGameplayTag StatTag, int32 DeltaCount)
{
	if (!StatTag.IsValid() || DeltaCount == 0)
	{
		return false;
	}

	FACInventoryEntry* FoundEntry = Entries.FindByPredicate([&InstanceId](const FACInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});

	if (!FoundEntry)
	{
		return false;
	}

	const int32 NewCount = FoundEntry->StatTagStacks.FindRef(StatTag) + DeltaCount;
	if (NewCount > 0)
	{
		FoundEntry->StatTagStacks.Add(StatTag, NewCount);
	}
	else
	{
		FoundEntry->StatTagStacks.Remove(StatTag);
	}

	CommitInventoryChange({ FoundEntry->ItemDefinition.Get() });
	return true;
}

bool UACInventorySubsystem::EquipItem(UACItemDefinition* ItemDefinition)
{
	UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	if (!MetaProgression)
	{
		return false;
	}

	if (!ItemDefinition)
	{
		ClearEquippedItem();
		return true;
	}

	if (!HasItem(ItemDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACInventorySubsystem] 보유하지 않은 아이템 %s를 장착하려 해 거부했습니다."), *ItemDefinition->GetName());
		return false;
	}

	if (!MetaProgression->SetEquippedItemDefinitionId(ItemDefinition->GetPrimaryAssetId()))
	{
		return true;
	}

	OnEquippedItemChangedDelegate.Broadcast(ItemDefinition);
	MetaProgression->RequestSave();
	return true;
}

void UACInventorySubsystem::ClearEquippedItem()
{
	UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	if (!MetaProgression || !MetaProgression->SetEquippedItemDefinitionId(FPrimaryAssetId()))
	{
		return;
	}

	OnEquippedItemChangedDelegate.Broadcast(nullptr);
	MetaProgression->RequestSave();
}

UACItemDefinition* UACInventorySubsystem::GetEquippedItemDefinition() const
{
	return ResolveItemDefinition(GetEquippedItemDefinitionId());
}

FPrimaryAssetId UACInventorySubsystem::GetEquippedItemDefinitionId() const
{
	const UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	return MetaProgression ? MetaProgression->GetEquippedItemDefinitionId() : FPrimaryAssetId();
}

TArray<FACInventoryEntry> UACInventorySubsystem::GetEntriesByCategory(FGameplayTag CategoryTag) const
{
	TArray<FACInventoryEntry> MatchingEntries;
	if (!CategoryTag.IsValid())
	{
		return MatchingEntries;
	}

	for (const FACInventoryEntry& Entry : Entries)
	{
		if (Entry.ItemDefinition && Entry.ItemDefinition->ItemCategoryTag.MatchesTag(CategoryTag))
		{
			MatchingEntries.Add(Entry);
		}
	}

	return MatchingEntries;
}

int32 UACInventorySubsystem::GetTotalItemCount(const UACItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return 0;
	}

	int32 TotalCount = 0;
	for (const FACInventoryEntry& Entry : Entries)
	{
		if (Entry.ItemDefinition == ItemDefinition)
		{
			TotalCount += Entry.StackCount;
		}
	}

	return TotalCount;
}

bool UACInventorySubsystem::HasItem(const UACItemDefinition* ItemDefinition) const
{
	return GetTotalItemCount(ItemDefinition) > 0;
}

const FACInventoryEntry* UACInventorySubsystem::FindEntryByInstanceId(const FGuid& InstanceId) const
{
	return Entries.FindByPredicate([&InstanceId](const FACInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});
}

#if WITH_EDITOR
void UACInventorySubsystem::SetItemDefinitionResolverForTests(TFunction<UACItemDefinition*(const FPrimaryAssetId&)> InResolver)
{
	ItemDefinitionResolverForTests = MoveTemp(InResolver);
}

void UACInventorySubsystem::InitializeForTests(UACMetaProgressionSubsystem* InMetaProgression)
{
	MetaProgressionForTests = InMetaProgression;

	if (InMetaProgression)
	{
		InMetaProgression->OnActiveSlotChangedDelegate.AddDynamic(this, &UACInventorySubsystem::HandleActiveSlotChanged);
	}

	RebuildEntriesFromSaveData();
}
#endif

void UACInventorySubsystem::HandleActiveSlotChanged(int32 NewSlotIndex)
{
	RebuildEntriesFromSaveData();
}

void UACInventorySubsystem::CommitInventoryChange(const TArray<UACItemDefinition*>& ChangedDefinitions)
{
	UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	if (!MetaProgression)
	{
		return;
	}

	// 1) 저장용 구조체 갱신 -> 2) 변경 통지 -> 3) 활성 슬롯이 있을 때만 실제 저장
	MetaProgression->SetSavedInventoryEntries(BuildSavedEntries());

	for (UACItemDefinition* ChangedDefinition : ChangedDefinitions)
	{
		if (ChangedDefinition)
		{
			OnInventoryChangedDelegate.Broadcast(ChangedDefinition, GetTotalItemCount(ChangedDefinition));
		}
	}

	MetaProgression->RequestSave();
}

void UACInventorySubsystem::RebuildEntriesFromSaveData()
{
	// 슬롯이 바뀌면 이전 슬롯에만 있던 아이템도 UI에서 사라져야 한다.
	// 그러려면 "사라진 정의"를 알아야 하므로 갈아엎기 전에 이전 총량을 찍어 둔다.
	TMap<UACItemDefinition*, int32> PreviousTotals;
	GatherTotalsByDefinition(PreviousTotals);

	Entries.Reset();
	UnresolvedSavedEntries.Reset();

	const UACMetaProgressionSubsystem* MetaProgression = GetMetaProgression();

	int32 DroppedCount = 0;
	int32 SplitEntryCount = 0;

	if (MetaProgression)
	{
		for (const FACSavedInventoryEntry& SavedEntry : MetaProgression->GetSavedInventoryEntries())
		{
			if (!SavedEntry.IsValid())
			{
				// ID가 비었거나 수량이 0 이하인 항목은 되살릴 값이 없으므로 완전히 버린다
				++DroppedCount;
				continue;
			}

			UACItemDefinition* ItemDefinition = ResolveItemDefinition(SavedEntry.ItemDefinitionId);
			if (!ItemDefinition)
			{
				// 에셋이 삭제·개명되었거나 아직 스캔되지 않은 항목이다.
				// 이 항목만 인벤토리에서 빼되 저장 데이터에는 그대로 남겨 두어, 에셋이 돌아오면 함께 돌아오게 한다
				UnresolvedSavedEntries.Add(SavedEntry);
				continue;
			}

			const int32 MaxStack = ItemDefinition->GetEffectiveMaxStackCount();
			int32 RemainingCount = FMath::Max(0, SavedEntry.StackCount);

			// 원본 항목: InstanceId와 개별 상태(강화/수치)를 그대로 물려받는다
			const int32 FirstCount = FMath::Min(RemainingCount, MaxStack);
			RemainingCount -= FirstCount;

			FACInventoryEntry& RestoredEntry = Entries.AddDefaulted_GetRef();
			RestoredEntry.ItemDefinition = ItemDefinition;
			RestoredEntry.StackCount = FirstCount;
			RestoredEntry.InstanceId = SavedEntry.InstanceId.IsValid() ? SavedEntry.InstanceId : FGuid::NewGuid();
			RestoredEntry.StatTagStacks = SavedEntry.StatTagStacks;
			RestoredEntry.UpgradeLevel = FMath::Max(0, SavedEntry.UpgradeLevel);
			// 여기서부터 Entries에 원소를 더 추가하므로 RestoredEntry 참조를 다시 쓰면 안 된다

			// MaxStackCount가 줄어든 뒤에 열린 옛 저장이면 상한을 넘는 수량이 남는다.
			// 잘라 버리면 플레이어 아이템이 조용히 사라지므로 여분은 기본 상태 항목으로 쪼개 보존한다.
			// 강화·수치 상태는 복제하지 않는다 — 복제하면 상한 축소가 강화 아이템 증식 수단이 된다.
			while (RemainingCount > 0)
			{
				if (Entries.Num() >= ACInventoryInternal::MaxInventoryEntries)
				{
					UE_LOG(LogTemp, Warning, TEXT("[ACInventorySubsystem] 항목 수 상한(%d) 때문에 %s의 남은 수량 %d개를 복원하지 못했습니다. 저장 데이터는 그대로 두므로 상한을 늘리면 되살아납니다."),
						ACInventoryInternal::MaxInventoryEntries, *ItemDefinition->GetName(), RemainingCount);
					break;
				}

				FACInventoryEntry& OverflowEntry = Entries.AddDefaulted_GetRef();
				OverflowEntry.ItemDefinition = ItemDefinition;
				OverflowEntry.InstanceId = FGuid::NewGuid();
				OverflowEntry.StackCount = FMath::Min(RemainingCount, MaxStack);
				RemainingCount -= OverflowEntry.StackCount;

				++SplitEntryCount;
			}
		}
	}

	if (DroppedCount > 0 || UnresolvedSavedEntries.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACInventorySubsystem] 해석할 수 없는 인벤토리 항목 %d개를 건너뛰고(저장 데이터에는 보존) 손상된 항목 %d개를 버렸습니다. 정상 복원 %d개."), UnresolvedSavedEntries.Num(), DroppedCount, Entries.Num());
	}

	if (SplitEntryCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACInventorySubsystem] 스택 상한이 줄어든 저장 데이터를 만나 항목 %d개로 나눠 복원했습니다. 수량은 그대로 보존됩니다."), SplitEntryCount);
	}

	// 여기서부터 Entries는 이미 새 슬롯의 내용이다 — 통지를 받은 쪽이 GetAllEntries()를 읽으면 새 데이터가 보인다
	TMap<UACItemDefinition*, int32> NewTotals;
	GatherTotalsByDefinition(NewTotals);

	// 새 슬롯에 있는 아이템: 복원된 총수량을 알린다
	for (const TPair<UACItemDefinition*, int32>& NewTotal : NewTotals)
	{
		OnInventoryChangedDelegate.Broadcast(NewTotal.Key, NewTotal.Value);
	}

	// 이전 슬롯에만 있던 아이템: 0을 알려야 UI 슬롯이 남지 않는다
	for (const TPair<UACItemDefinition*, int32>& PreviousTotal : PreviousTotals)
	{
		if (!NewTotals.Contains(PreviousTotal.Key))
		{
			OnInventoryChangedDelegate.Broadcast(PreviousTotal.Key, 0);
		}
	}

	OnEquippedItemChangedDelegate.Broadcast(GetEquippedItemDefinition());
}

TArray<FACSavedInventoryEntry> UACInventorySubsystem::BuildSavedEntries() const
{
	TArray<FACSavedInventoryEntry> SavedEntries;
	SavedEntries.Reserve(Entries.Num() + UnresolvedSavedEntries.Num());

	for (const FACInventoryEntry& Entry : Entries)
	{
		if (!Entry.ItemDefinition || Entry.StackCount <= 0)
		{
			continue;
		}

		FACSavedInventoryEntry& SavedEntry = SavedEntries.AddDefaulted_GetRef();
		SavedEntry.ItemDefinitionId = Entry.ItemDefinition->GetPrimaryAssetId();
		SavedEntry.StackCount = Entry.StackCount;
		SavedEntry.InstanceId = Entry.InstanceId;
		SavedEntry.StatTagStacks = Entry.StatTagStacks;
		SavedEntry.UpgradeLevel = Entry.UpgradeLevel;
	}

	// 이번 실행에서 해석하지 못한 항목도 원본 그대로 다시 써서 보존한다
	SavedEntries.Append(UnresolvedSavedEntries);

	return SavedEntries;
}

UACItemDefinition* UACInventorySubsystem::ResolveItemDefinition(const FPrimaryAssetId& ItemDefinitionId)
{
	if (!ItemDefinitionId.IsValid())
	{
		return nullptr;
	}

	#if WITH_EDITOR
	if (ItemDefinitionResolverForTests)
	{
		return ItemDefinitionResolverForTests(ItemDefinitionId);
	}
	#endif

	if (!UAssetManager::IsInitialized())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	// 이미 로드되어 있으면 그대로 쓴다
	if (UACItemDefinition* LoadedDefinition = AssetManager.GetPrimaryAssetObject<UACItemDefinition>(ItemDefinitionId))
	{
		return LoadedDefinition;
	}

	// 아직 로드 전이면 경로를 얻어 동기 로드한다. 스캔 규칙이 없거나 에셋이 사라졌으면 무효 경로가 돌아온다
	const FSoftObjectPath DefinitionPath = AssetManager.GetPrimaryAssetPath(ItemDefinitionId);
	return DefinitionPath.IsValid() ? Cast<UACItemDefinition>(DefinitionPath.TryLoad()) : nullptr;
}

UACMetaProgressionSubsystem* UACInventorySubsystem::GetMetaProgression() const
{
	#if WITH_EDITOR
	if (MetaProgressionForTests.IsValid())
	{
		return MetaProgressionForTests.Get();
	}
	#endif

	UGameInstance* OwningGameInstance = GetGameInstance();
	return OwningGameInstance ? OwningGameInstance->GetSubsystem<UACMetaProgressionSubsystem>() : nullptr;
}
