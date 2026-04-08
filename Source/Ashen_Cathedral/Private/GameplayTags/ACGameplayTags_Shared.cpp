// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayTags/ACGameplayTags_Shared.h"

namespace ACGameplayTags
{
	#pragma region Shared Event Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_MeleeHit, "Shared.Event.MeleeHit")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_GroggyTriggered, "Shared.Event.GroggyTriggered")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_Death, "Shared.Event.Death")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_HitReact, "Shared.Event.HitReact")
	#pragma endregion

	#pragma region Shared Status Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Groggy, "Shared.Status.Groggy")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Dead, "Shared.Status.Dead")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Invincible, "Shared.Status.Invincible")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_SuperArmor, "Shared.Status.SuperArmor")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Sprinting, "Shared.Status.Sprinting")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_CanCounterAttack, "Shared.Status.CanCounterAttack")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact, "Shared.Status.HitReact")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Front, "Shared.Status.HitReact.Front")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Left, "Shared.Status.HitReact.Left")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Back, "Shared.Status.HitReact.Back")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Right, "Shared.Status.HitReact.Right")
	#pragma endregion

	#pragma region Shared SetByCaller Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BaseDamage, "Shared.SetByCaller.BaseDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_CounterAttackBonus, "Shared.SetByCaller.CounterAttackBonus")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_GroggyDamage, "Shared.SetByCaller.GroggyDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_AttackType_Light, "Shared.SetByCaller.AttackType.Light")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_AttackType_Heavy, "Shared.SetByCaller.AttackType.Heavy")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_FireBonusDamage, "Shared.SetByCaller.FireBonusDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BurnBuildUp, "Shared.SetByCaller.BurnBuildUp")
	#pragma endregion

	#pragma region Shared Abilies Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_HitReact, "Shared.Ability.HitReact")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_Death, "Shared.Ability.Death")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_BurnDot, "Shared.Ability.BurnDot")
	#pragma endregion

	#pragma region Shared Event Tags - Burn
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_BurnTriggered, "Shared.Event.BurnTriggered")
	#pragma endregion
}