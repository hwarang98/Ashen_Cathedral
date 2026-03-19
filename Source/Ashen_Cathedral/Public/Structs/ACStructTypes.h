// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "ACStructTypes.generated.h"

class AACWeaponBase;

class UACPlayerGameplayAbility;


/**
 * @struct FCAInputActionConfig
 * @brief 입력 액션과 관련된 구성을 정의하는 데이터 구조체.
 *
 * 이 구조체는 게임플레이 태그와 해당 태그에 매핑된 입력 액션을 설정하는 데 사용한다.
 * 주로 입력 설정 데이터 에셋이나 입력 행위를 바인딩할 때 활용한다.
 */
USTRUCT(BlueprintType)
struct FCAInputActionConfig
{
	GENERATED_BODY()


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction;

	bool IsValid() const;
};

/**
 * @struct FACPlayerAbilitySet
 * @brief 플레이어의 어빌리티 설정을 정의하는 데이터 구조체.
 *
 * 이 구조체는 특정 입력 태그와 UI 슬롯 태그에 따라 플레이어가 사용할 수 있는 어빌리티를 정의한다.
 * 어빌리티 부여, UI 아이콘 참조, 쿨다운 태그 설정 등에 사용한다.
 *
 * @details
 * - InputTag: 입력 시스템과 연관된 태그.
 * - SlotTag: UI 슬롯과 매핑에 사용되는 태그.
 * - AbilityToGrant: 설정된 어빌리티 클래스.
 * - SoftAbilityIconMaterial: 어빌리티 UI 아이콘을 위한 에셋.
 * - CooldownTag: 어빌리티의 쿨다운을 트래킹하는데 사용되는 태그.
 */
USTRUCT(BlueprintType)
struct FACPlayerAbilitySet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	// UI 슬롯 태그 (예: InputTag_Skill_Q) - 캐릭터 클래스와 무관하게 UI 슬롯 매핑용
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Player.Skill"))
	// FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UACPlayerGameplayAbility> AbilityToGrant;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly)
	// TSoftObjectPtr<UTexture2D> SoftAbilityIconMaterial;

	// 쿨다운 태그 (GE에서 사용하는 CooldownTag)
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Cooldown"))
	// FGameplayTag CooldownTag;

	bool IsValid() const;
};

USTRUCT()
struct FWeaponEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag WeaponTag;

	UPROPERTY()
	TObjectPtr<AACWeaponBase> WeaponActor;
};