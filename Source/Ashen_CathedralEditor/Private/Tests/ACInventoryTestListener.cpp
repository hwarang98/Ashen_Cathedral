// 인벤토리 변경 델리게이트 수신을 검증하기 위한 테스트 전용 리스너

#include "Tests/ACInventoryTestListener.h"

#include "Subsystems/ACInventorySubsystem.h"

int32 UACInventoryTestListener::FindLastCountFor(const UACItemDefinition* ItemDefinition) const
{
	for (int32 Index = ReceivedInventoryChanges.Num() - 1; Index >= 0; --Index)
	{
		if (ReceivedInventoryChanges[Index].Key == ItemDefinition)
		{
			return ReceivedInventoryChanges[Index].Value;
		}
	}

	return INDEX_NONE;
}

int32 UACInventoryTestListener::CountNotificationsFor(const UACItemDefinition* ItemDefinition) const
{
	int32 NotificationCount = 0;
	for (const TPair<UACItemDefinition*, int32>& Change : ReceivedInventoryChanges)
	{
		if (Change.Key == ItemDefinition)
		{
			++NotificationCount;
		}
	}

	return NotificationCount;
}

void UACInventoryTestListener::ResetRecords()
{
	ReceivedInventoryChanges.Reset();
	EquippedChangeCount = 0;
	LastEquippedItem = nullptr;
	EntryCountAtLastNotify = INDEX_NONE;
}

void UACInventoryTestListener::HandleInventoryChanged(UACItemDefinition* ItemDefinition, int32 NewTotalCount)
{
	ReceivedInventoryChanges.Emplace(ItemDefinition, NewTotalCount);

	// 통지를 받은 시점에 인벤토리가 이미 새 상태여야 한다는 계약을 확인하기 위한 스냅샷
	if (ObservedInventory)
	{
		EntryCountAtLastNotify = ObservedInventory->GetAllEntries().Num();
	}
}

void UACInventoryTestListener::HandleEquippedItemChanged(UACItemDefinition* NewEquippedItem)
{
	++EquippedChangeCount;
	LastEquippedItem = NewEquippedItem;
}
