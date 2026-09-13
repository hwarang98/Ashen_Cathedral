// 슬롯 세이브에 기록되는 인벤토리 항목 1개 — 런타임 UObject/Actor를 참조하지 않는 순수 직렬화 데이터

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "ACSavedInventoryEntry.generated.h"

/**
 * @brief 인벤토리 Entry 하나의 영속 데이터.
 *
 * 아이템 원형은 UACItemDefinition 에셋을 직접 들지 않고 FPrimaryAssetId로만 가리킨다.
 * 에셋이 삭제되거나 이름이 바뀌면 그 Entry만 로드에서 건너뛰고 나머지는 정상 복원된다.
 * 액터 포인터, 위젯 포인터, GameplayAbilitySpecHandle, 활성 GameplayEffect 핸들은 절대 여기에 넣지 않는다.
 */
USTRUCT(BlueprintType)
struct FACSavedInventoryEntry
{
	GENERATED_BODY()

	// 아이템 원형 식별자. UACItemDefinition::GetPrimaryAssetId()가 돌려준 값 그대로 저장한다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FPrimaryAssetId ItemDefinitionId;

	// 이 Entry가 보유한 수량. 1 미만이 되면 Entry 자체가 제거된다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 StackCount = 0;

	// Entry를 개별 식별하는 키. 스택되지 않는 아이템(MaxStackCount == 1)을 여러 개 들 때 서로 구분하는 데 쓴다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid InstanceId;

	// 아이템별 자유 수치 스택 (Item.Stat.*). 강화 단계와 달리 아이템마다 의미가 다르다
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TMap<FGameplayTag, int32> StatTagStacks;

	// 강화 단계. 0이면 미강화
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 UpgradeLevel = 0;

	// ItemDefinitionId가 유효하고 수량이 1 이상이면 참
	bool IsValid() const
	{
		return ItemDefinitionId.IsValid() && StackCount > 0;
	}
};
