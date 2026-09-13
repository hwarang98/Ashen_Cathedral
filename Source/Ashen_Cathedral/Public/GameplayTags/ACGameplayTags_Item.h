// 인벤토리 아이템 분류 GameplayTag 선언 — UACItemDefinition::ItemCategoryTag와 인벤토리 조회 필터에 사용한다

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	#pragma region Item Category Tags
	// 아이템 분류 태그. UI 필터와 인벤토리 조회(GetEntriesByCategory)의 기준이며, 하위 태그를 추가해도 상위 태그로 걸린다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Weapon);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Consumable);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Material);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Key);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Relic);
	#pragma endregion

	#pragma region Item Stat Tags
	// 아이템 인스턴스가 보유하는 수치 스택 키(FACSavedInventoryEntry::StatTagStacks). 강화 단계와 달리 아이템별 자유 수치다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Stat_Durability);
	#pragma endregion
}
