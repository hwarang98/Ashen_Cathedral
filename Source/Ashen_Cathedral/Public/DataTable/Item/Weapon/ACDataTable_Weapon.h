// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Enums/Weapon/ACWeaponEnum.h"
#include "ACDataTable_Weapon.generated.h"

USTRUCT(BlueprintType)
struct ASHEN_CATHEDRAL_API FACWeaponStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "무기 이름")
	FText WeaponName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "무기 타입")
	EACWeaponType WeaponType = EACWeaponType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "데미지")
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "약공격 체간 데미지")
	float LightAttackPostureDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "강공격 체간 데미지")
	float HeavyAttackPostureDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "카운터 체간 데미지")
	float CounterAttackPostureDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "공격 속도")
	float AttackSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "스태미나 소모")
	float StaminaCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "비고")
	FString Note = TEXT("");
};