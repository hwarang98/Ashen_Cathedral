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

float UPlayerCombatComponent::GetPlayerCurrentWeaponLightPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->LightAttackPostureDamage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponHeavyPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->HeavyAttackPostureDamage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponCounterPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->CounterAttackPostureDamage;
	}
	return 0.f;
}

EACWeaponType UPlayerCombatComponent::GetPlayerCurrentWeaponType() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->WeaponType;
	}
	return EACWeaponType::None;
}

float UPlayerCombatComponent::GetCurrentWeaponBaseDamage() const
{
	return GetPlayerCurrentEquippedWeaponDamageAtLevel();
}

float UPlayerCombatComponent::GetCurrentWeaponLightAttackPostureDamage() const
{
	return GetPlayerCurrentWeaponLightPostureDamage();
}

float UPlayerCombatComponent::GetCurrentWeaponHeavyAttackPostureDamage() const
{
	return GetPlayerCurrentWeaponHeavyPostureDamage();
}

float UPlayerCombatComponent::GetCurrentWeaponCounterAttackPostureDamage() const
{
	return GetPlayerCurrentWeaponCounterPostureDamage();
}

float UPlayerCombatComponent::GetCurrentWeaponAttackSpeed() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->AttackSpeed;
	}
	return 1.f;
}