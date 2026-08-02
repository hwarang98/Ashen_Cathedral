// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "Structs/ACStructTypes.h"
#include "ACDataAsset_WeaponData.generated.h"

class UNiagaraSystem;
class UGameplayEffect;
class AACWeapon;
class AACWeaponBase;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Socket")
	TSoftObjectPtr<UTexture2D> SoftWeaponIconTexture;

	// 이 무기를 장착할 때 ASC에 추가할 무기 식별 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FGameplayTag WeaponTypeTag;

	// 이 무기를 스폰할 때 사용할 액터 클래스. 로비 프리뷰와 실제 무기 스폰이 모두 이 값을 사용한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AACWeapon> WeaponClassToSpawn;

	// 스폰 직후 부착할 소켓. 비어 있으면 UnequippedSocketName을 대신 사용한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Socket")
	FName InitialSocketName;

	// 무기 스폰과 함께 부여할 장착 어빌리티. 장착 전에도 입력을 받아야 하므로 DefaultWeaponAbilities와 수명이 다르다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FACPlayerAbilitySet EquipAbility;

	// 장착이 확정되는 순간 무기 메시에 붙여 재생할 나이아가라. 비어 있어도 장착은 정상 진행된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> EquipNiagaraSystem;
};