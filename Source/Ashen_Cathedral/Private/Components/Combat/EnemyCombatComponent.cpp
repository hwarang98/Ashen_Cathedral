// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/EnemyCombatComponent.h"
#include "DataTable/Item/Weapon/ACDataTable_Weapon.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
#include "Items/Weapon/ACWeapon.h"

const FACWeaponStatRow* UEnemyCombatComponent::GetCurrentWeaponStatRow() const
{
	if (const AACWeapon* Weapon = Cast<AACWeapon>(GetCharacterCurrentEquippedWeapon()))
	{
		if (const UACDataAsset_WeaponData* WeaponData = Weapon->WeaponData)
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

float UEnemyCombatComponent::GetCurrentWeaponHeavyAttackGroggyDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->HeavyAttackGroggyDamage;
	}
	return 0.f;
}

float UEnemyCombatComponent::GetCurrentWeaponCounterAttackGroggyDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->CounterAttackGroggyDamage;
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
