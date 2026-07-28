// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/ACGameplayTags_Enemy.h"

namespace ACGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Enemy_AshenKnight_Weapon_Sword, "Enemy.AshenKnight.Weapon.Sword")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ordan_Weapon_Lance, "Enemy.Ordan.Weapon.Lance")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Strafing, "Enemy.Status.Strafing")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Dodging, "Enemy.Status.Dodging")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_UnderAttack, "Enemy.Status.UnderAttack")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_PressureCountering, "Enemy.Status.PressureCountering")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Attacking, "Enemy.Status.Attacking")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Blocking, "Enemy.Status.Blocking")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Melee, "Enemy.Ability.Melee")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Dodge, "Enemy.Ability.Dodge")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Phase2, "Enemy.Ability.Phase2")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_AttackType_Run, "Enemy.Ability.AttackType.Run")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_AttackType_BackDash, "Enemy.Ability.AttackType.BackDash")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Pressure_Counter, "Enemy.Ability.Pressure.Counter")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_AttackType_Special_01, "Enemy.Ability.AttackType.Special.01")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_AttackType_Special_02, "Enemy.Ability.AttackType.Special.02")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_AttackType_Special_03, "Enemy.Ability.AttackType.Special.03")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Block, "Enemy.Ability.Block")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Parry, "Enemy.Ability.Parry")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Parry_CounterAttack, "Enemy.Ability.Parry.CounterAttack")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Event_Dodge, "Enemy.Event.Dodge")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Phase2, "Enemy.Status.Phase2")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_State_Phase2, "Enemy.State.Phase2")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Event_Phase2_VisualActivate, "Enemy.Event.Phase2.VisualActivate")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Attack_Fire, "Enemy.Attack.Fire")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Attack_Phase2, "Enemy.Attack.Phase2")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_SetByCaller_FireBonusDamage, "Enemy.SetByCaller.FireBonusDamage")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_SetByCaller_BurnBuildUp, "Enemy.SetByCaller.BurnBuildUp")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Event_PressureDetected, "Enemy.Event.PressureDetected")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_State_PressureReady, "Enemy.State.PressureReady")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Event_ParrySuccess, "Enemy.Event.ParrySuccess")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_CriticalAttack, "Enemy.Ability.CriticalAttack")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_CriticalAttacking, "Enemy.Status.CriticalAttacking")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_StateTree_Event_TargetAcquired, "Enemy.StateTree.Event.TargetAcquired")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_StateTree_Event_TargetLost, "Enemy.StateTree.Event.TargetLost")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_StateTree_Event_PressureReady, "Enemy.StateTree.Event.PressureReady")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_StateTree_Event_IncomingAttack, "Enemy.StateTree.Event.IncomingAttack")

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Cooldown_Attack, "Enemy.Cooldown.Attack")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Cooldown_DashAttack, "Enemy.Cooldown.DashAttack")
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Cooldown_BackDashAttack, "Enemy.Cooldown.BackDashAttack")
}