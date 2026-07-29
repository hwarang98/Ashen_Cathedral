// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/ACGameplayTags_Combat.h"

namespace ACGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_Combat_IncomingAttack, "Shared.Event.Combat.IncomingAttack")

	UE_DEFINE_GAMEPLAY_TAG(Shared_Attack_Blockable, "Shared.Attack.Blockable")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Attack_Parryable, "Shared.Attack.Parryable")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Attack_Unblockable, "Shared.Attack.Unblockable")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Attack_Unparryable, "Shared.Attack.Unparryable")
	UE_DEFINE_GAMEPLAY_TAG(Shared_Attack_Weight_Heavy, "Shared.Attack.Weight.Heavy")

	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Stagger, "Shared.Status.Stagger")
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_StaggerDuration, "Shared.SetByCaller.StaggerDuration")
}
