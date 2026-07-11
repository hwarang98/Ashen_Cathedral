// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/EnemyCombatComponent.h"
#include "DataTable/Item/Weapon/ACDataTable_Weapon.h"
#include "DataAssets/Items/Weapon/ACDataAsse_EnemyWeaponData.h"
#include "Items/Weapon/ACEnemyWeapon.h"

const FACWeaponStatRow* UEnemyCombatComponent::GetCurrentWeaponStatRow() const
{
	if (const AACEnemyWeapon* Weapon = Cast<AACEnemyWeapon>(GetCharacterCurrentEquippedWeapon()))
	{
		if (const UACDataAsse_EnemyWeaponData* WeaponData = Weapon->WeaponData)
		{
			return WeaponData->WeaponStatRow.GetRow<FACWeaponStatRow>(TEXT("GetCurrentWeaponStatRow"));
		}
	}
	return nullptr;
}

float UEnemyCombatComponent::GetCurrentWeaponBaseDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->Damage;
	}
	return 0.f;
}

float UEnemyCombatComponent::GetCurrentWeaponLightAttackPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->LightAttackPostureDamage;
	}
	return 0.f;
}

float UEnemyCombatComponent::GetCurrentWeaponHeavyAttackPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->HeavyAttackPostureDamage;
	}
	return 0.f;
}

float UEnemyCombatComponent::GetCurrentWeaponCounterAttackPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->CounterAttackPostureDamage;
	}
	return 0.f;
}

float UEnemyCombatComponent::GetCurrentWeaponAttackSpeed() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->AttackSpeed;
	}
	return 1.f;
}