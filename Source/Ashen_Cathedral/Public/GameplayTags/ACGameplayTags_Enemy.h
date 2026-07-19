// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_AshenKnight_Weapon_Sword)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Strafing)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Dodging)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_UnderAttack)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Attacking)          // 공격 어빌리티(Melee/Special 등) 활성화 중 부여되는 상태 태그. BT의 ActivateAbilityByTagAndWait가 대기 조건으로 사용할 수 있다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_PressureCountering) // Pressure Counter 어빌리티가 실제로 실행 중인 상태. Dodge와 상호 배제(ActivationBlockedTags)에 사용된다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Blocking)           // GA_BossBlock 활성화 중 부여되는 상태 태그. Shared.Attack.Blockable 판정에서 UACFunctionLibrary::IsActorBlocking이 검사한다.

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Melee)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Dodge)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Phase2)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_AttackType_Run)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Pressure_Counter)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_AttackType_Special_01)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_AttackType_Special_02)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_AttackType_Special_03)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Block)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Parry)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Parry_CounterAttack)


	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Status_Phase2)               // Phase2 상태 태그. Phase2 어빌리티가 활성화된 동안 ASC에 부여됩니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_State_Phase2)                // Ashen Knight Phase2 진입 상태 태그. GA_AshenKnight_Phase2 활성화 중 부여됩니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Event_Phase2_VisualActivate) // Phase2 전환 몽타주의 AnimNotify에서 발송하는 이벤트. 수신 시 머티리얼/Niagara를 적용합니다.


	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Attack_Fire)	// Phase2 공격 속성 태그. 공격 어빌리티에서 화염/Phase2 분기 처리에 사용합니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Attack_Phase2)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_SetByCaller_FireBonusDamage)	// Phase2 SetByCaller 태그. 화염 추가 데미지와 화상 축적량 전달에 사용합니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_SetByCaller_BurnBuildUp)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Event_PressureDetected)	// PressureDetection 컴포넌트가 짧은 시간 내 히트 임계치 도달 시 발송하는 이벤트. Response Ability가 AbilityTriggers로 구독한다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Event_Dodge)

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_State_PressureReady)	// 압박 반응이 요청된 상태(BT 진입 조건). PressureDetection이 부여하고, 실제로 반응 Ability(Counter/Dodge)가 시작되면 그 Ability가 제거한다.

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Event_ParrySuccess)	// ACCalculation_DamageTaken이 Boss의 Shared.Status.Parry 판정 성공 시 발송하는 이벤트. GA_BossParry가 WaitGameplayEvent로 대기한다.
}