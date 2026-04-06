// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_AshenKnight_Weapon_Sword)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Strafing)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_UnderAttack)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Melee)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Phase2)

	// Phase2 상태 태그. Phase2 어빌리티가 활성화된 동안 ASC에 부여됩니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Phase2)
	// Ashen Knight Phase2 진입 상태 태그. GA_AshenKnight_Phase2 활성화 중 부여됩니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_State_Phase2)
	// Phase2 전환 몽타주의 AnimNotify에서 발송하는 이벤트. 수신 시 머티리얼/Niagara를 적용합니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Event_Phase2_VisualActivate)

	// Phase2 공격 속성 태그. 공격 어빌리티에서 화염/Phase2 분기 처리에 사용합니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Attack_Fire)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Attack_Phase2)

	// Phase2 SetByCaller 태그. 화염 추가 데미지와 화상 축적량 전달에 사용합니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_SetByCaller_FireBonusDamage)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_SetByCaller_BurnBuildUp)
}