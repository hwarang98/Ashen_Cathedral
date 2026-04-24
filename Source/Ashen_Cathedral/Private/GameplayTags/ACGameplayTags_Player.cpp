// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/ACGameplayTags_Player.h"

namespace ACGameplayTags
{
	#pragma region Player Ability Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_EquipWeapon, "Player.Ability.EquipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_UnEquipWeapon, "Player.Ability.UnEquipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Sprint, "Player.Ability.Sprint")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Attack_Light, "Player.Ability.Attack.Light")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Attack_Heavy, "Player.Ability.Attack.Heavy")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Block, "Player.Ability.Block")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Roll, "Player.Ability.Roll")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_TargetLock, "Player.Ability.TargetLock")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Execution, "Player.Ability.Execution")
	#pragma endregion

	#pragma region Player Weapon Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Weapon_Sword, "Player.Weapon.Sword")
	#pragma endregion

	#pragma region Player Status Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Equipping, "Player.Status.Equipping")
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Stamina_RegenBlocked, "Player.Status.Stamina.RegenBlocked")
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Blocking, "Player.Status.Blocking")
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Rolling, "Player.Status.Rolling")
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_TargetLock, "Player_Status_TargetLock")
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Executing, "Player.Status.Executing")
	#pragma endregion

	#pragma region Player SendGameplayEvent Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_EquipWeapon, "Player.Event.EquipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_UnequipWeapon, "Player.Event.UnequipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_SuccessfulBlock, "Player.Event.SuccessfulBlock")
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_SwitchTarget_Left, "Player.Event.SwitchTarget.Left")
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_SwitchTarget_Right, "Player.Event.SwitchTarget.Right")

	#pragma endregion
}