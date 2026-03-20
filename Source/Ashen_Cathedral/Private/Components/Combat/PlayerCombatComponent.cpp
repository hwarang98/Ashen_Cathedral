// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/PlayerCombatComponent.h"
#include "Items/Weapon/ACWeapon.h"

UPlayerCombatComponent::UPlayerCombatComponent() {}

AACWeapon* UPlayerCombatComponent::GetPlayerCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AACWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}