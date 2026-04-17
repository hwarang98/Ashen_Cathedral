// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACGameplayAbility_Groggy.generated.h"

/**
 * 플레이어·적 공용 그로기 어빌리티.
 *
 * AttributeSet이 GroggyGauge >= MaxGroggyGauge를 감지하면
 * Shared.Event.GroggyTriggered 이벤트를 발사하고,
 * 이 어빌리티가 트리거되어 그로기 상태를 처리한다.
 *
 * 어빌리티 활성 중에는 ActivationOwnedTags로 Shared_Status_Groggy가 자동 부여되며,
 * EndAbility 시 자동으로 제거된다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayAbility_Groggy : public UACGameplayAbility
{
	GENERATED_BODY()

public:
	UACGameplayAbility_Groggy();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 그로기 상태에서 재생할 몽타주 목록 (랜덤 선택) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groggy|Animation")
	TArray<TObjectPtr<UAnimMontage>> GroggyMontages;

private:
	/** 몽타주가 정상 종료됐을 때 호출 — GroggyGauge 리셋 후 어빌리티 종료 */
	UFUNCTION()
	void OnMontageCompleted();

	/** 몽타주가 중단됐을 때 호출 — 어빌리티 즉시 종료 */
	UFUNCTION()
	void OnMontageCancelled();
};
