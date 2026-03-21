// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/PlayerCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
#include "Items/Weapon/ACWeapon.h"

UPlayerCombatComponent::UPlayerCombatComponent() {}

AACWeapon* UPlayerCombatComponent::GetPlayerCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AACWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}

AACWeapon* UPlayerCombatComponent::GetPlayerCurrentEquippedWeapon() const
{
	AACWeapon* PlayerWeapon = Cast<AACWeapon>(GetCharacterCurrentEquippedWeapon());

	return PlayerWeapon ? PlayerWeapon : nullptr;
}

const UACDataAsset_WeaponData* UPlayerCombatComponent::GetPlayerCurrentWeaponData() const
{
	if (AACWeapon* PlayerWeapon = GetPlayerCurrentEquippedWeapon())
	{
		return PlayerWeapon->WeaponData;
	}
	return nullptr;
}

const FACWeaponStatRow* UPlayerCombatComponent::GetCurrentWeaponStatRow() const
{
	if (const AACWeapon* PlayerWeapon = GetPlayerCurrentEquippedWeapon())
	{
		if (const UACDataAsset_WeaponData* WeaponData = PlayerWeapon->WeaponData)
		{
			return WeaponData->WeaponStatRow.GetRow<FACWeaponStatRow>(TEXT("GetCurrentWeaponStatRow"));
		}
	}
	return nullptr;
}

float UPlayerCombatComponent::GetPlayerCurrentEquippedWeaponDamageAtLevel() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->Damage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponHeavyGroggyDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->HeavyAttackGroggyDamage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponCounterGroggyDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->CounterAttackGroggyDamage;
	}
	return 0.f;
}