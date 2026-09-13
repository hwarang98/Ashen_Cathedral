// 런타임 인벤토리 항목 1개 — 해석이 끝난 ItemDefinition 포인터를 들고 있어 UI/게임플레이가 바로 쓸 수 있다

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ACInventoryEntry.generated.h"

class UACItemDefinition;

/**
 * @brief UACInventorySubsystem이 메모리에 들고 있는 인벤토리 항목.
 *
 * 세이브용 FACSavedInventoryEntry와 1:1로 대응하지만, 이쪽은 FPrimaryAssetId 대신 해석된 정의 포인터를 들고 있다.
 * 세이브 파일에 들어가는 것은 언제나 FACSavedInventoryEntry 쪽이며 이 구조체는 직렬화되지 않는다.
 */
USTRUCT(BlueprintType)
struct FACInventoryEntry
{
	GENERATED_BODY()

	// 이 항목의 아이템 원형. 로드 시 해석에 실패한 항목은 애초에 만들어지지 않으므로 항상 유효하다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UACItemDefinition> ItemDefinition;

	// 이 항목이 보유한 수량. 항상 1 이상이며 0이 되면 항목 자체가 제거된다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 StackCount = 0;

	// 항목을 개별 식별하는 키. 스택되지 않는 아이템을 여러 개 들 때 서로 구분한다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid InstanceId;

	// 아이템별 자유 수치 스택 (Item.Stat.*)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TMap<FGameplayTag, int32> StatTagStacks;

	// 강화 단계. 0이면 미강화
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 UpgradeLevel = 0;
};
