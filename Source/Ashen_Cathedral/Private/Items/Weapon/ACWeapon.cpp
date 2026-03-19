// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/ACWeapon.h"
#include "AbilitySystemComponent.h"

void AACWeapon::AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InSpecHandles)
{
	GrantedAbilitySpecHandles = InSpecHandles;
}

const TArray<FGameplayAbilitySpecHandle>& AACWeapon::GetGrantedAbilitySpecHandles() const
{
	return GrantedAbilitySpecHandles;
}