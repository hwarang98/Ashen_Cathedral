// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	#pragma region Player Ability Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_EquipWeapon);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_UnEquipWeapon);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_Sprint);
	#pragma endregion

	#pragma region Player Weapon Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Weapon_Sword);
	#pragma endregion

	#pragma region Player Status Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Status_Equipping);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Status_Stamina_RegenBlocked);
	#pragma endregion

	#pragma region Player SetByCaller Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_SetByCaller_AttackType_Light);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_SetByCaller_AttackType_Heavy);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_SetByCaller_StaminaCost);
	#pragma endregion

	#pragma region Player SendGameplayEvent Tags
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Event_EquipWeapon);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Event_UnequipWeapon);
	#pragma endregion
}