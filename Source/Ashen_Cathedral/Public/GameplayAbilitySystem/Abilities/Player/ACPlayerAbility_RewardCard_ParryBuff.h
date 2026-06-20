// P01 반격의 본능 — 패링 성공 시 다음 공격 피해를 증가시키는 Passive GameplayAbility

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerAbility_RewardCard_ParryBuff.generated.h"

class UAbilityTask_WaitGameplayTagAdded;

/**
 * @brief P01 반격의 본능 — Passive GA.
 *
 * OnGiven 정책으로 부여 즉시 활성화되며, Shared.Status.CanCounterAttack 태그가
 * 추가될 때마다(패링 성공 시) BP에서 설정한 ParryAttackBuffEffect를 플레이어 ASC에 적용한다.
 *
 * GE_RewardCard_P01_ParryAttackBuff 를 Duration 방식으로 구성하면 카운터어택 윈도우 동안만
 * 공격력 버프가 유지된다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_RewardCard_ParryBuff : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerAbility_RewardCard_ParryBuff();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/**
	 * 패링 성공 시 적용할 공격력 버프 GameplayEffect.
	 * Duration 방식으로 설정하고 AttackPower에 +18% 보정을 적용한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParryBuff")
	TSubclassOf<UGameplayEffect> ParryAttackBuffEffect;

private:
	// Shared.Status.CanCounterAttack 태그 추가를 대기하는 Task 시작
	void StartWaitingForParryTag();

	// Shared.Status.CanCounterAttack 태그가 추가됐을 때 콜백
	UFUNCTION()
	void OnParryTagAdded();

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayTagAdded> WaitTagTask;
};
