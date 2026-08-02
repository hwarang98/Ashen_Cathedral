// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/ACWeapon.h"
#include "AbilitySystemComponent.h"

void AACWeapon::SetWeaponData(UACDataAsset_WeaponData* InWeaponData)
{
	WeaponData = InWeaponData;
}

void AACWeapon::AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InSpecHandles)
{
	GrantedAbilitySpecHandles = InSpecHandles;
}

const TArray<FGameplayAbilitySpecHandle>& AACWeapon::GetGrantedAbilitySpecHandles() const
{
	return GrantedAbilitySpecHandles;
}

void AACWeapon::SetEquipAbilitySpecHandle(const FGameplayAbilitySpecHandle& InSpecHandle)
{
	EquipAbilitySpecHandle = InSpecHandle;
}

const FGameplayAbilitySpecHandle& AACWeapon::GetEquipAbilitySpecHandle() const
{
	return EquipAbilitySpecHandle;
}

bool AACWeapon::HasValidEquipAbilitySpecHandle() const
{
	return EquipAbilitySpecHandle.IsValid();
}

void AACWeapon::ClearEquipAbilitySpecHandle()
{
	EquipAbilitySpecHandle = FGameplayAbilitySpecHandle();
}