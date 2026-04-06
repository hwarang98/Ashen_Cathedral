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

	/**
	 * true이면 OnHitTarget에서 콤보 횟수를 SetByCaller로 전달해 데미지 보너스를 적용한다.
	 * 적 어빌리티는 false로 설정해 콤보 배율을 비활성화한다.
	 */
	bool bApplyComboDamageBonus = true;

	/** 재생할 공격 몽타주를 선택해 반환한다. 기본 구현은 CurrentComboCount 기반 순차 선택. */
	virtual UAnimMontage* SelectAttackMontage();

	/**
	 * @brief DamageEffect Spec이 타겟에 적용되기 직전에 호출되는 확장 포인트.
	 * 서브클래스는 이 함수를 오버라이드해 동일 Spec에 SetByCaller 값을 추가할 수 있다.
	 *
	 * @param SpecHandle  현재 빌드 중인 DamageEffect Spec 핸들
	 * @param HitActor    적중된 대상 액터
	 * @param BaseDamage  이 공격의 기본 데미지 값
	 * @note GE 적용 전에 호출되므로 SetByCaller 삽입에 적합하다.
	 */
	virtual void ModifyDamageSpec(const FGameplayEffectSpecHandle& SpecHandle, const AActor* HitActor, float BaseDamage) {}

	/**
	 * @brief 타겟 적중 후 추가 효과를 적용하기 위한 확장 포인트. 기본 구현은 빈 함수.
	 *
	 * @param HitActor 적중된 대상 액터
	 * @param Payload  Shared_Event_MeleeHit 이벤트 페이로드
	 * @note GE 적용 완료 후의 후처리(VFX 스폰 등)에 사용한다.
	 */
	virtual void ApplyAdditionalHitEffects(const AActor* HitActor, const FGameplayEventData& Payload) {}

	/** 순차적으로 재생할 공격 몽타주 배열 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

private:

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