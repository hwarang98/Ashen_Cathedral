// 압박 반격 예시 Ability — BT가 Enemy.State.PressureReady 태그를 보고 Enemy.Ability.Pressure.Counter 태그로 직접 활성화한다.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "ACEnemyAbility_PressureCounter.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAOEDamageComponent;

UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_PressureCounter : public UACEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UACEnemyAbility_PressureCounter();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 압박 반격 몽타주. 보스별 BP 서브클래스에서 지정한다. None이면 반격 없이 바로 종료한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|Montage")
	TObjectPtr<UAnimMontage> CounterMontage;

	/** 반격 재생 중 적용할 무적 GameplayEffect. None이면 적용하지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|Effects")
	TSubclassOf<UGameplayEffect> InvincibilityEffect;

	/** 반격이 적중했을 때 타겟에게 적용할 데미지 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|Effects")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 단발형 AOE 판정 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE", meta = (ClampMin = "0.0"))
	float InstantAOERadius = 200.f;

	/** 단발형 AOE 판정 원점을 Owner 전방으로 밀어낼 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE")
	float InstantAOEForwardOffset = 0.f;

	/** 지속형 AOE 판정 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE", meta = (ClampMin = "0.0"))
	float SustainedAOERadius = 150.f;

	/** 지속형 AOE 판정 원점을 Owner 전방으로 밀어낼 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE")
	float SustainedAOEForwardOffset = 0.f;

	/** 지속 중 AOE 스윕 판정을 반복할 간격 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE", meta = (ClampMin = "0.01"))
	float SustainedAOEDamageInterval = 0.05f;

	/** AOE 데미지 = 현재 무기 기본 데미지 * 이 배율 (Instant/Sustained 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE", meta = (ClampMin = "0.0"))
	float AOEBaseDamageMultiplier = 1.f;

	/** AOE 히트 시 각 타겟에게 주입할 그로기 데미지. 0이면 주입하지 않는다 (Instant/Sustained 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE", meta = (ClampMin = "0.0"))
	float AOEGroggyDamage = 0.f;

	/** true면 AOE 판정 범위(단발 스피어 / 지속형 스윕 경로)를 디버그로 표시한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|AOE")
	bool bDebugDrawAOE = false;

	/** 반격이 타겟에 적중했을 때 재생할 GameplayCue 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureCounter|GameplayCue")
	FGameplayTag HitGameplayCueTag;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FActiveGameplayEffectHandle InvincibilityEffectHandle;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	/** Shared.Event.MeleeHit 이벤트 수신 시 호출 — 타겟에게 DamageEffect를 적용한다 */
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

	/** 무적 GameplayEffect를 자신에게 적용 */
	void ApplyInvincibilityEffect();

	/**
	 * @brief Owner Character에서 UAOEDamageComponent를 찾고, 없으면 새로 부착해 반환한다.
	 * AOE 판정(오버랩/스윕/중복 방지/타이머)은 이 컴포넌트가 전담하며, 어빌리티는 GE 생성/적용만 담당한다.
	 */
	UAOEDamageComponent* GetOrCreateAOEDamageComponent() const;

	/**
	 * @brief DamageEffect Spec을 생성해 SetByCaller 값을 주입하고 타겟 ASC에 적용한다.
	 * OnHitTarget, OnInstantAOEEventReceived, OnSustainedAOEStartReceived가 공통으로 사용한다.
	 *
	 * @param TargetActor   적중된 대상 액터
	 * @param BaseDamage    Shared_SetByCaller_BaseDamage로 주입할 기본 데미지
	 * @param GroggyDamage  Shared_SetByCaller_GroggyDamage로 주입할 그로기 데미지. 0 이하면 주입하지 않는다.
	 * @return 타겟 ASC를 찾아 Spec을 적용했으면 true
	 */
	bool ApplyDamageEffectSpecToTarget(const AActor* TargetActor, float BaseDamage, float GroggyDamage);

	/**
	 * @brief HitGameplayCueTag를 HitActor 위치/방향으로 재생한다.
	 * ApplyDamageEffectSpecToTarget이 GE 적용 성공 후 공통으로 호출한다.
	 * @note 대상이 Block/Parry/Invincible/Dead 상태면 재생하지 않는다 — 실제로 맞은 효과가 없기 때문이다.
	 */
	void PlayHitGameplayCue(const AActor* HitActor) const;
};