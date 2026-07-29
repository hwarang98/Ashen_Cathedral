// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	#pragma region Shared Event Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_MeleeHit);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_PostureBrokenTriggered);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_GuardBrokenTriggered); // GuardGauge가 최대치에 도달했을 때 AttributeSet이 발송한다. Block 어빌리티가 수신해 가드 브레이크를 실행한다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_Death);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_HitReact);
	// 단발형 AOE 데미지 타이밍 이벤트 — 몽타주 AnimNotify가 1회 발송하면 즉시 범위 판정 후 DamageEffect를 적용한다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_AOE_Instant);
	// 지속형 AOE 시작 이벤트 — 몽타주 AnimNotifyState의 NotifyBegin에서 발송, 대상 구간 동안 스윕 판정을 시작한다 (대쉬/채널링/이동 스킬 등 재사용 가능)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_AOE_Sustained_Start);
	// 지속형 AOE 종료 이벤트 — 몽타주 AnimNotifyState의 NotifyEnd에서 발송, 스윕 판정을 종료한다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_AOE_Sustained_End);
	#pragma endregion

	#pragma region Shared Status Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_PostureBroken);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_Dead);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_Invincible);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_SuperArmor);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_Sprinting);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_CanCounterAttack);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_HitReact);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_Parry);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_HitReact_Front);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_HitReact_Left);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_HitReact_Back);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_HitReact_Right);
	// Enemy가 처형당하는 동안 부여되는 상태 태그 — HitReact/PostureDamage 차단 및 AI 잠금에 사용
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_Executed);
	// 체간 피해를 받은 직후 부여되는 상태 태그 — GE_PostureDecay의 Ongoing Tag Requirement가 이 태그 보유 중엔 자연 감소를 막는다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_PostureDecayBlocked);
	// 가드로 막아낸 직후 부여되는 상태 태그 — GE_GuardGaugeDecay의 Ongoing Tag Requirement가 이 태그 보유 중엔 자연 감소를 막는다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_GuardDecayBlocked);
	#pragma endregion

	#pragma region Shared SetByCaller Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_BaseDamage);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_CounterAttackBonus);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_PostureDamage);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_GuardDamage); // 막아낸 공격의 가드 부하량을 GuardDamageEffect에 전달한다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_AttackType_Light);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_AttackType_Heavy);
	// Phase2 화염 추가 데미지. DamageCalculation에서 BaseDamage에 합산됩니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_FireBonusDamage);
	// Phase2 화상 축적량. DamageCalculation에서 BurnAccumulation 메타 Attribute에 출력됩니다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_BurnBuildUp);
	// 다이나믹 쿨다운 GE의 Duration을 런타임에 주입하는 SetByCaller 태그
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_CooldownDuration);
	#pragma endregion

	#pragma region Shared Abilies Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Ability_HitReact);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Ability_Death);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Ability_BurnDot);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Ability_PostureBroken);
	#pragma endregion

	#pragma region Shared Event Tags - Burn
	// BurnGauge가 MaxBurnGauge에 도달했을 때 DoT 어빌리티를 트리거하는 이벤트 태그
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_BurnTriggered);
	#pragma endregion

	#pragma region CriticalAttack Tags
	// 크리티컬 어택 몽타주의 AnimNotify가 발송하는 데미지 타이밍 이벤트 — 이 이벤트 수신 시 CriticalAttackDamage GE를 적용한다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_CriticalAttackDamage);
	// CriticalAttackDamageEffect GE에서 SetByCaller로 크리티컬 어택 데미지 값을 전달하는 태그
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_CriticalAttackDamage);
	#pragma endregion
}