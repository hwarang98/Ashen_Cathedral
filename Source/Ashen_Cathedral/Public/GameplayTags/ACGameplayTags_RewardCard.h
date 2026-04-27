// 카드 보상 시스템에서 사용하는 GameplayTag 선언

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	#pragma region RewardCard Ability Tags
	// P01 패링 성공 시 공격력 버프를 부여하는 Passive Ability 태그
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_RewardCard_ParryBuff);
	#pragma endregion

	#pragma region RewardCard Status Tags
	// 카드 선택 UI가 활성화된 동안 플레이어 ASC에 부여되는 상태 태그 — 중복 UI 방지에 사용
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Status_RewardCard_SelectionActive);
	#pragma endregion
}
