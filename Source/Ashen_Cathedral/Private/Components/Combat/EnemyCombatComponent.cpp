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
