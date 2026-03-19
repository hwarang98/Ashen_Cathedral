// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "UACAbility_SpawnWeapon.generated.h"

class AACWeaponBase;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UUACAbility_SpawnWeapon : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UUACAbility_SpawnWeapon();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/* 스폰할 무기 액터 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AACWeaponBase> WeaponClassToSpawn;

	/* 스폰된 무기를 등록할 게임플레이 태그 (Equip GA에서 이 태그를 사용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FGameplayTag WeaponTagToRegister;

	/* 스폰 직후 무기를 부착할 초기 소켓 (예: Back_Socket) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName InitialSocketName;

	/* 스폰과 동시에 장착할지 여부 (true = 즉시 장착, false = 등록만) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bRegisterAsEquippedWeapon = false;

};