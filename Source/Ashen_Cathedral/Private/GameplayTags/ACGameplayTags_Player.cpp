// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/ACGameplayTags_Player.h"

namespace ACGameplayTags
{
	#pragma region Player Ability Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_EquipWeapon, "Player.Ability.EquipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_UnEquipWeapon, "Player.Ability.UnEquipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Sprint, "Player.Ability.Sprint")
	#pragma endregion

	#pragma region Player Weapon Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Weapon_Sword, "Player.Weapon.Sword")
	#pragma endregion

	#pragma region Player Status Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Equipping, "Player.Status.Equipping")
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Stamina_RegenBlocked, "Player.Status.Stamina.RegenBlocked")
	#pragma endregion

	#pragma region Player SendGameplayEvent Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_EquipWeapon, "Player.Event.EquipWeapon")
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_UnequipWeapon, "Player.Event.UnequipWeapon")
	#pragma endregion

	#pragma region Player SetByCaller Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_SetByCaller_AttackType_Light, "Player.SetByCaller.AttackType.Light")
	UE_DEFINE_GAMEPLAY_TAG(Player_SetByCaller_AttackType_Heavy, "Player.SetByCaller.AttackType.Heavy")
	UE_DEFINE_GAMEPLAY_TAG(Player_SetByCaller_StaminaCost, "Player.SetByCaller.StaminaCost")
	#pragma endregion
}