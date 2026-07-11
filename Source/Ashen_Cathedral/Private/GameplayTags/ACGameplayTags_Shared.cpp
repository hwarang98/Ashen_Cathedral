// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayTags/ACGameplayTags_Shared.h"

namespace ACGameplayTags
{
	#pragma region Shared Event Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_MeleeHit, "Shared.Event.MeleeHit")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_PostureBrokenTriggered, "Shared.Event.PostureBrokenTriggered")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_Death, "Shared.Event.Death")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_HitReact, "Shared.Event.HitReact")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_AOE_Instant, "Shared.Event.AOE.Instant")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_AOE_Sustained_Start, "Shared.Event.AOE.Sustained.Start")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_AOE_Sustained_End, "Shared.Event.AOE.Sustained.End")
	#pragma endregion

	#pragma region Shared Status Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_PostureBroken, "Shared.Status.PostureBroken")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Dead, "Shared.Status.Dead")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Invincible, "Shared.Status.Invincible")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_SuperArmor, "Shared.Status.SuperArmor")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Sprinting, "Shared.Status.Sprinting")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_CanCounterAttack, "Shared.Status.CanCounterAttack")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact, "Shared.Status.HitReact")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Parry, "Shared.Status.Parry")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Front, "Shared.Status.HitReact.Front")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Left, "Shared.Status.HitReact.Left")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Back, "Shared.Status.HitReact.Back")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Right, "Shared.Status.HitReact.Right")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Executed, "Shared.Status.Executed")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_PostureDecayBlocked, "Shared.Status.PostureDecayBlocked")
	#pragma endregion

	#pragma region Shared SetByCaller Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BaseDamage, "Shared.SetByCaller.BaseDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_CounterAttackBonus, "Shared.SetByCaller.CounterAttackBonus")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_PostureDamage, "Shared.SetByCaller.PostureDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_AttackType_Light, "Shared.SetByCaller.AttackType.Light")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_AttackType_Heavy, "Shared.SetByCaller.AttackType.Heavy")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_FireBonusDamage, "Shared.SetByCaller.FireBonusDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BurnBuildUp, "Shared.SetByCaller.BurnBuildUp")
	#pragma endregion

	#pragma region Shared Abilies Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_HitReact, "Shared.Ability.HitReact")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_Death, "Shared.Ability.Death")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_BurnDot, "Shared.Ability.BurnDot")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_PostureBroken, "Shared.Ability.PostureBroken")
	#pragma endregion

	#pragma region Shared Event Tags - Burn
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_BurnTriggered, "Shared.Event.BurnTriggered")
	#pragma endregion

	#pragma region CriticalAttack Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_CriticalAttackDamage, "Shared.Event.CriticalAttackDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_CriticalAttackDamage, "Shared.SetByCaller.CriticalAttackDamage")
	#pragma endregion
}