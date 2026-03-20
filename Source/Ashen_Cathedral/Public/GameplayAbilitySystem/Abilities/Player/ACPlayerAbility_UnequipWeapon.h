// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerAbility_UnequipWeapon.generated.h"

/**
 * 무기를 '해제'하는 어빌리티.
 * CanActivateAbility를 통해 무기를 들고 있을 때만 활성화됩니다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_UnequipWeapon : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerAbility_UnequipWeapon();
	/* 어빌리티 활성화 가능 여부를 확인 (무기를 들고 있어야 함) */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/* 어빌리티를 활성화 (몽타주 재생 및 해제 로직) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/* 몽타주 재생이 정상적으로 완료/블렌드아웃 되었을 때 호출 */
	UFUNCTION()
	void OnMontageEnded();

	/* 몽타주가 취소되거나 중단되었을 때 호출 */
	UFUNCTION()
	void OnMontageCancelled();

	/* 실제 해제 로직을 처리하는 함수 */
	UFUNCTION()
	void HandleUnequipLogic(FGameplayEventData Payload);

	/* 무기를 해제할 때 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> UnequipMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event", meta = (AllowPrivateAccess=true))
	FGameplayTag UnequipEventTag;
};