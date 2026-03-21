// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "DataTable/Item/Weapon/ACDataTable_Weapon.h"
#include "PlayerCombatComponent.generated.h"

class AACWeapon;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UPlayerCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	UPlayerCombatComponent();

	// 태그로 소지 중인 무기를 반환
	AACWeapon* GetPlayerCarriedWeaponByTag(FGameplayTag InWeaponTag) const;

	// 현재 장착된 무기를 반환
	AACWeapon* GetPlayerCurrentEquippedWeapon() const;

	// 현재 장착된 무기의 DataAsset을 반환
	const UACDataAsset_WeaponData* GetPlayerCurrentWeaponData() const;

	// 현재 장착 무기의 레벨별 기본 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Damage")
	float GetPlayerCurrentEquippedWeaponDamageAtLevel() const;

	// 현재 장착 무기의 강공격 그로기 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Groggy")
	float GetPlayerCurrentWeaponHeavyGroggyDamage() const;

	// 현재 장착 무기의 카운터 그로기 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Groggy")
	float GetPlayerCurrentWeaponCounterGroggyDamage() const;

private:
	// 현재 장착 무기의 DataTable Row를 반환
	const FACWeaponStatRow* GetCurrentWeaponStatRow() const;
};