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
	#pragma endregion

	#pragma region Shared SetByCaller Tags
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BaseDamage, "Shared.SetByCaller.BaseDamage")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_CounterAttackBonus, "Shared.SetByCaller.CounterAttackBonus")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_GroggyDamage, "Shared.SetByCaller.GroggyDamage")
	#pragma endregion
}