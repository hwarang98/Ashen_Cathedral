// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACGameplayTags.h"
#include "ACAbility_Attack.generated.h"

class UAOEDamageComponent;
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
	#pragma region Combo
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
	#pragma endregion

	#pragma region Damage Extension Points
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
	#pragma endregion

	#pragma region Damage Effect Helpers
	/**
	 * @brief DamageEffect Spec을 생성하고 BaseDamage/GroggyDamage SetByCaller 값을 주입한다.
	 * OnHitTarget, OnInstantAOEEventReceived, OnSustainedAOEStartReceived가 공통으로 사용하는 GE 생성 로직이다.
	 *
	 * @param BaseDamage    Shared_SetByCaller_BaseDamage로 주입할 기본 데미지
	 * @param GroggyDamage  Shared_SetByCaller_GroggyDamage로 주입할 그로기 데미지. 0 이하면 주입하지 않는다.
	 * @return DamageEffect가 없거나 Spec 생성에 실패하면 Invalid 핸들을 반환한다.
	 */
	FGameplayEffectSpecHandle CreateDamageEffectSpec(float BaseDamage, float GroggyDamage);

	/**
	 * @brief ModifyDamageSpec 확장 포인트를 호출한 뒤 Spec을 타겟 ASC에 적용한다.
	 *
	 * @param SpecHandle  CreateDamageEffectSpec으로 생성한 Spec 핸들
	 * @param HitActor    적중된 대상 액터
	 * @param BaseDamage  ModifyDamageSpec에 전달할 기본 데미지 값
	 * @return 타겟 ASC를 찾아 Spec을 적용했으면 true
	 */
	bool ApplyDamageEffectSpecToTarget(const FGameplayEffectSpecHandle& SpecHandle, const AActor* HitActor, float BaseDamage);

	/**
	 * @brief MeleeAttackSoundCueTag GameplayCue를 HitActor 위치/방향으로 재생한다.
	 * OnHitTarget, OnInstantAOEEventReceived, OnSustainedAOEStartReceived가 GE 적용 성공 후 공통으로 호출한다.
	 * @note Parry/Block으로 최종 데미지가 0이거나 감소해도 큐는 재생된다 — 명중 자체에 대한 피드백이기 때문이다.
	 */
	void PlayHitGameplayCue(const AActor* HitActor) const;
	#pragma endregion

	#pragma region Montage
	/** 순차적으로 재생할 공격 몽타주 배열 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;
	#pragma endregion

private:
	#pragma region Montage
	/** 카운터 어택 몽타주 — 여러 개 등록 시 매번 랜덤으로 하나를 선택해 재생한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> CounterAttackMontages;

	/** CounterAttackMontages 중 하나를 랜덤으로 선택해 반환한다. 비어있으면 nullptr. */
	UAnimMontage* SelectCounterAttackMontage() const;
	#pragma endregion

	#pragma region Damage Effect
	/** 타겟에게 적용할 데미지 게임플레이 이펙트 (서버 전용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffect;
	#pragma endregion

	#pragma region Combo State
	/** 현재 콤보 횟수 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combo", meta = (AllowPrivateAccess = "true"))
	int32 CurrentComboCount = 0;

	/**
	 * ActivateAbility 시점에 판정한 카운터어택 여부를 캐시한다.
	 * OnHitTarget은 이 값을 사용해야 한다 — Shared_Status_CanCounterAttack 태그는
	 * ActivateAbility에서 이미 소모(제거)되므로, 히트 시점에 태그를 다시 조회하면
	 * 항상 false로 읽혀 카운터 데미지 보너스가 누락된다.
	 */
	bool bWasCounterAttack = false;

	/** 콤보 공격 타입 (Light/Heavy) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Attack", meta = (AllowPrivateAccess = "true", Categories = "Shared.SetByCaller"))
	FGameplayTag ComboAttackTypeTag;

	/** 카운터 어택 성공 시 데미지에 곱해지는 배율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Attack", meta = (AllowPrivateAccess = "true"))
	float CounterAttackDamageMultiplier = 1.5f;
	#pragma endregion

	#pragma region GameplayCue & Camera
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCue", meta = (AllowPrivateAccess = "true"))
	FGameplayTag MeleeAttackSoundCueTag;

	/** 공격이 타겟에 적중했을 때 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CameraShake", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;
	#pragma endregion

	#pragma region AOE
	/** 단발형 AOE 판정 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE|Instant", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float InstantAOERadius = 200.f;

	/** 단발형 AOE 판정 원점을 Owner 전방으로 밀어낼 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE|Instant", meta = (AllowPrivateAccess = "true"))
	float InstantAOEForwardOffset = 0.f;

	/** 지속형 AOE 판정 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE|Sustained", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SustainedAOERadius = 150.f;

	/** 지속형 AOE 판정 원점을 Owner 전방으로 밀어낼 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE|Sustained", meta = (AllowPrivateAccess = "true"))
	float SustainedAOEForwardOffset = 0.f;

	/** 지속 중 AOE 스윕 판정을 반복할 간격 (초). 짧을수록 빠른 이동 중 누락이 줄어든다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE|Sustained", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float SustainedAOEDamageInterval = 0.05f;

	/** AOE 데미지 = 현재 무기 기본 데미지 * 이 배율 (Shared_SetByCaller_BaseDamage로 주입, Instant/Sustained 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AOEBaseDamageMultiplier = 1.f;

	/** AOE 히트 시 각 타겟에게 주입할 그로기 데미지. 0이면 주입하지 않는다 (Instant/Sustained 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AOEGroggyDamage = 0.f;

	/** true면 AOE 판정 범위(단발 스피어 / 지속형 스윕 경로)를 디버그로 표시한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE", meta = (AllowPrivateAccess = "true"))
	bool bDebugDrawAOE = false;

	/**
	 * @brief Owner Character에서 UAOEDamageComponent를 찾고, 없으면 새로 부착해 반환한다.
	 * AOE 판정(오버랩/스윕/중복 방지/타이머)은 이 컴포넌트가 전담하며, 어빌리티는 GE 생성/적용만 담당한다.
	 */
	UAOEDamageComponent* GetOrCreateAOEDamageComponent() const;
	#pragma endregion

	#pragma region Montage Callbacks
	/** 몽타주 재생이 정상적으로 완료/블렌드아웃되었을 때 호출 */
	UFUNCTION()
	void OnMontageEnded();

	/** 몽타주가 취소되거나 중단되었을 때 호출 */
	UFUNCTION()
	void OnMontageCancelled();
	#pragma endregion

	#pragma region Gameplay Event Callbacks
	/** 'Shared_Event_MeleeHit' 이벤트를 수신했을 때 호출 */
	UFUNCTION()
	void OnHitTarget(FGameplayEventData Payload);

	/**
	 * @brief 'Shared_Event_AOE_Instant' 이벤트를 수신했을 때 호출.
	 * 서버 권한에서 UAOEDamageComponent::TriggerInstantAOE를 호출해 1회 판정하고,
	 * 찾아낸 대상마다 DamageEffect Spec을 만들어 적용한다.
	 */
	UFUNCTION()
	void OnInstantAOEEventReceived(FGameplayEventData Payload);

	/**
	 * @brief 'Shared_Event_AOE_Sustained_Start' 이벤트를 수신했을 때 호출.
	 * UAOEDamageComponent::StartSustainedAOE를 호출해 지속형 스윕 판정을 시작한다.
	 */
	UFUNCTION()
	void OnSustainedAOEStartReceived(FGameplayEventData Payload);

	/**
	 * @brief 'Shared_Event_AOE_Sustained_End' 이벤트를 수신했을 때 호출.
	 * UAOEDamageComponent::StopSustainedAOE를 호출해 지속형 판정을 종료한다.
	 */
	UFUNCTION()
	void OnSustainedAOEEndReceived(FGameplayEventData Payload);
	#pragma endregion

};
