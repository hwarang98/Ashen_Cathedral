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

	// 현재 장착 무기의 약공격 체간 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Posture")
	float GetPlayerCurrentWeaponLightPostureDamage() const;

	// 현재 장착 무기의 강공격 체간 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Posture")
	float GetPlayerCurrentWeaponHeavyPostureDamage() const;

	// 현재 장착 무기의 카운터 체간 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Posture")
	float GetPlayerCurrentWeaponCounterPostureDamage() const;

	// 현재 장착 무기의 타입을 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon")
	EACWeaponType GetPlayerCurrentWeaponType() const;

	virtual float GetCurrentWeaponBaseDamage() const override;
	virtual float GetCurrentWeaponLightAttackPostureDamage() const override;
	virtual float GetCurrentWeaponHeavyAttackPostureDamage() const override;
	virtual float GetCurrentWeaponCounterAttackPostureDamage() const override;
	virtual float GetCurrentWeaponAttackSpeed() const override;

private:
	// 현재 장착 무기의 DataTable Row를 반환
	const FACWeaponStatRow* GetCurrentWeaponStatRow() const;
};