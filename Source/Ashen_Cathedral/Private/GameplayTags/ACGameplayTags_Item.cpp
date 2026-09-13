// 인벤토리 아이템 분류 GameplayTag 정의

#include "GameplayTags/ACGameplayTags_Item.h"

namespace ACGameplayTags
{
	#pragma region Item Category Tags
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Weapon, "Item.Category.Weapon")
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Consumable, "Item.Category.Consumable")
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Material, "Item.Category.Material")
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Key, "Item.Category.Key")
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Relic, "Item.Category.Relic")
	#pragma endregion

	#pragma region Item Stat Tags
	UE_DEFINE_GAMEPLAY_TAG(Item_Stat_Durability, "Item.Stat.Durability")
	#pragma endregion
}
