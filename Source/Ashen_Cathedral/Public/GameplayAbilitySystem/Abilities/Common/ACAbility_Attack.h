// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACGameplayTags.h"
#include "ACAbility_Attack.generated.h"

class UCameraShakeBase;

/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACAbility_Attack : public UACGameplayAbility
{
	GENERATED_BODY()

public:
	UACAbility_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	/**
	 * 콤보가 정상 완료되었을 때 호출 (자식 클래스가 구현)
	 * - BaseAttack: 리셋 타이머 시작
	 * - SkillCombo: 아무것도 안함 (리셋 없음)
	 */
	virtual void HandleComboComplete();

	/**
	 * 콤보가 취소되었을 때 호출 (자식 클래스가 구현)
	 * - BaseAttack: 즉시 리셋
	 * - SkillCombo: 즉시 리셋 또는 아무것도 안함
	 */
	virtual void HandleComboCancelled();

	/** 콤보 카운트를 리셋한다. 자식 클래스에서 호출 가능. */
	void ResetComboCount();

private:
	/** 순차적으로 재생할 공격 몽타주 배열 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** 카운터 어택 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> CounterAttackMontage;

	/** 타겟에게 적용할 데미지 게임플레이 이펙트 (서버 전용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 현재 콤보 횟수 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combo", meta = (AllowPrivateAccess = "true"))
	int32 CurrentComboCount = 0;

	/** 콤보 공격 타입 (Light/Heavy) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Attack", meta = (AllowPrivateAccess = "true", Categories = "Shared.SetByCaller"))
	FGameplayTag ComboAttackTypeTag;

	/** 카운터 어택 성공 시 데미지에 곱해지는 배율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Attack", meta = (AllowPrivateAccess = "true"))
	float CounterAttackDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCue", meta = (AllowPrivateAccess = "true"))
	FGameplayTag MeleeAttackSoundCueTag;

	/** 공격이 타겟에 적중했을 때 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CameraShake", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	/** 몽타주 재생이 정상적으로 완료/블렌드아웃되었을 때 호출 */
	UFUNCTION()
	void OnMontageEnded();

	/** 몽타주가 취소되거나 중단되었을 때 호출 */
	UFUNCTION()
	void OnMontageCancelled();

	/** 'Shared_Event_MeleeHit' 이벤트를 수신했을 때 호출 */
	UFUNCTION()
	void OnHitTarget(FGameplayEventData Payload);

};