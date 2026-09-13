// 인벤토리 변경 델리게이트 수신을 검증하기 위한 테스트 전용 리스너 — 다이내믹 델리게이트라 UObject가 필요하다

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ACInventoryTestListener.generated.h"

class UACInventorySubsystem;
class UACItemDefinition;

/**
 * @brief OnInventoryChanged / OnEquippedItemChanged 브로드캐스트를 기록하는 테스트용 객체.
 *
 * @note 다이내믹 멀티캐스트 델리게이트는 UFUNCTION에만 바인딩되므로 람다로는 받을 수 없다.
 *       게임에는 쓰이지 않는다.
 */
UCLASS(NotBlueprintable, Hidden)
class UACInventoryTestListener : public UObject
{
	GENERATED_BODY()

public:
	// 수량 변경 통지를 받은 순서대로 기록한다 (같은 정의가 여러 번 올 수 있다)
	TArray<TPair<UACItemDefinition*, int32>> ReceivedInventoryChanges;

	// 장착 변경 통지 횟수
	int32 EquippedChangeCount = 0;

	// 마지막으로 통지받은 장착 아이템
	UPROPERTY()
	TObjectPtr<UACItemDefinition> LastEquippedItem;

	/**
	 * @brief 통지 시점에 인벤토리가 이미 새 상태인지 확인하기 위해, 콜백 안에서 조회한 항목 수를 남긴다.
	 * 구독 시 설정해 두면 매 통지마다 갱신된다.
	 */
	UPROPERTY()
	TObjectPtr<UACInventorySubsystem> ObservedInventory;

	// 마지막 통지 시점에 ObservedInventory->GetAllEntries()가 돌려준 항목 수. 구독하지 않았으면 -1
	int32 EntryCountAtLastNotify = INDEX_NONE;

	// 특정 정의에 대해 마지막으로 통지받은 수량. 통지가 없었으면 INDEX_NONE
	int32 FindLastCountFor(const UACItemDefinition* ItemDefinition) const;

	// 특정 정의에 대한 통지 횟수
	int32 CountNotificationsFor(const UACItemDefinition* ItemDefinition) const;

	// 기록을 비운다
	void ResetRecords();

	UFUNCTION()
	void HandleInventoryChanged(UACItemDefinition* ItemDefinition, int32 NewTotalCount);

	UFUNCTION()
	void HandleEquippedItemChanged(UACItemDefinition* NewEquippedItem);
};
