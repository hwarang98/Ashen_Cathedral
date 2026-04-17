// 플레이어 전용 Block 어빌리티 — SuccessfulBlock 이벤트 처리, 패링 판정, 카운터어택 윈도우, RootMotion, GameplayCue를 담당한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACGameplayAbility_Block.h"
#include "ACPlayerAbility_Block.generated.h"

class UAbilityTask_WaitGameplayEvent;

UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_Block : public UACGameplayAbility_Block
{
	GENERATED_BODY()

public:
	UACPlayerAbility_Block();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** 패링 성공 후 카운터어택 가능 윈도우 유지 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Block|CounterAttack")
	float CounterAttackWindowDuration = 2.0f;

	/** 블록 히트 시 밀려나는 힘 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|RootMotion")
	float HitPushbackStrength = 200.f;

	/** 블록 히트 RootMotion 지속 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Block|RootMotion")
	float HitPushbackDuration = 0.2f;

private:
	/** 몽타주가 정상 종료(완료/블렌드아웃)됐을 때 호출 */
	UFUNCTION()
	void OnMontageCompleted();

	/** 몽타주가 취소/중단됐을 때 호출 */
	UFUNCTION()
	void OnMontageCancelled();

	/** 몽타주 블렌드인 완료 시 GameplayCue 추가 및 RootMotion 시작 */
	UFUNCTION()
	void OnMontageBlendedIn();

	/**
	 * @brief Player.Event.SuccessfulBlock 이벤트 수신 시 호출
	 * Shared.Status.Parry 태그 보유 여부에 따라 패링/블락 처리를 분기한다.
	 */
	UFUNCTION()
	void OnSuccessfulBlockEventReceived(FGameplayEventData Payload);

	/** 카운터어택 윈도우 타이머 만료 시 호출 — Shared.Status.CanCounterAttack 태그 제거 */
	UFUNCTION()
	void ResetCounterAttackWindow();

	/** Payload의 Instigator 방향으로 액터를 Yaw 회전시킨다 */
	void RotateActorToTargetFromEventData(const FGameplayEventData& Payload) const;

	/** 블록 히트 이벤트 기반으로 GameplayCue Parameters를 구성한다 */
	FGameplayCueParameters MakeBlockGameplayCueParams(const FGameplayEventData& Payload) const;

	void ExecuteSuccessfulBlockCue(const FGameplayEventData& Payload);
	void ExecuteParryCue(const FGameplayEventData& Payload);

	/** 패링 성공 시 Shared.Status.CanCounterAttack 태그를 추가하고 윈도우 타이머를 시작한다 */
	void StartCounterAttackWindow();

	FGameplayEventData CachedPayload;
	FTimerHandle CounterAttackTimerHandle;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;
};