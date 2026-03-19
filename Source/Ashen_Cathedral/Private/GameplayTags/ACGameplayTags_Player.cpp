// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/ACGameplayTags_Player.h"

namespace ACGameplayTags
{
	#pragma region Player Ability Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_EquipWeapon, "Player.Ability.EquipWeapon")
	#pragma endregion

	#pragma region Player Weapon Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Weapon_Sword, "Player.Weapon.Sword")
	#pragma endregion

	#pragma region Player Status Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Equipping, "Player.Status.Equipping")
	#pragma endregion

	#pragma region Player SendGameplayEvent Tags
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_EquipWeapon, "Player.Event.EquipWeapon")
	#pragma endregion
}