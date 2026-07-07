// 압박 반격 예시 Ability — BT가 Enemy.State.PressureReady 태그를 보고 Enemy.Ability.Pressure.Counter 태그로 직접 활성화한다.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "ACEnemyAbility_PressureCounter.generated.h"

class UAbilityTask_PlayMontageAndWait;

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

	/** 무적 GameplayEffect를 자신에게 적용 */
	void ApplyInvincibilityEffect();
};