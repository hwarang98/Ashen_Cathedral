// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "ACDataAsset_WeaponData.generated.h"

class UNiagaraSystem;
class UGameplayEffect;
class AACWeaponBase;
struct FACPlayerAbilitySet;
class UInputMappingContext;
class UACPlayerLinkedAnimLayer;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACDataAsset_WeaponData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 무기를 장착했을때 재생시킬 애님BP
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UACPlayerLinkedAnimLayer> WeaponAnimLayerToLink;

	// 입력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> WeaponInputMappingContext;

	// 장비의 어빌리티
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (TitleProperty = "InputTag"))
	TArray<FACPlayerAbilitySet> DefaultWeaponAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FDataTableRowHandle WeaponStatRow;

	// 무기가 소횐되는 소켓
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Socket")
	FName EquippedSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Socket")
	FName UnequippedSocketName;

	// 이 무기의 스킬 애니메이션 몽타주에서 사용하는 나이아가라 파티클들, 무기 장착 시 미리 로드하여 스킬 첫 사용 시 프리징 방지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TArray<TSoftObjectPtr<UNiagaraSystem>> SkillParticleSystems;
};