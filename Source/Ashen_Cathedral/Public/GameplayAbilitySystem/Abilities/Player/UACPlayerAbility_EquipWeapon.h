// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "UACPlayerAbility_EquipWeapon.generated.h"

/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UUACPlayerAbility_EquipWeapon : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UUACPlayerAbility_EquipWeapon();

	/* 어빌리티 활성화 가능 여부를 확인합니다. (무기를 들고 있지 않아야 함) */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/* 어빌리티를 활성화 (몽타주 재생 및 장착 로직) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/* 몽타주 재생이 정상적으로 완료/블렌드아웃 되었을 때 호출 */
	UFUNCTION()
	void OnMontageEnded();

	/* 몽타주가 취소되거나 중단되었을 때 호출 */
	UFUNCTION()
	void OnMontageCancelled();

	/* 실제 장착 로직을 처리하는 함수 */
	UFUNCTION()
	void HandleEquipLogic(FGameplayEventData Payload);

	/* 무기를 장착할 때 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> EquipMontage;

	/* 이 어빌리티가 장착할 무기의 태그 (PawnCombatComponent에 등록된 태그) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess=true))
	FGameplayTag WeaponToEquipTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event", meta = (AllowPrivateAccess=true))
	FGameplayTag EquipEventTag;
};