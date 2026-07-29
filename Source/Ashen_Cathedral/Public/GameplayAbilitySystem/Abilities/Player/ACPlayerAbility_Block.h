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
	/**
	 * 패링 성공 시 적용할 카운터어택 윈도우 GameplayEffect.
	 * Duration Policy: Has Duration, Granted Tag: Shared.Status.CanCounterAttack로 구성한다.
	 * 지속시간 만료 시 태그 제거를 ASC가 자동으로 처리하므로 별도 타이머 관리가 필요 없다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|CounterAttack")
	TSubclassOf<UGameplayEffect> CounterAttackWindowEffect;

	/** 블록 히트 시 밀려나는 힘 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|RootMotion")
	float HitPushbackStrength = 200.f;

	/** 블록 히트 RootMotion 지속 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Block|RootMotion")
	float HitPushbackDuration = 0.2f;

	/**
	 * 막아낼 때마다 GuardGauge에 부하를 넣는 Instant GE.
	 * Modifier: GuardDamageTaken (Add, SetByCaller: Shared.SetByCaller.GuardDamage)로 구성한다.
	 * 비우면 가드 게이지가 누적되지 않는다(가드 브레이크 기능 비활성).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GuardBreak")
	TSubclassOf<UGameplayEffect> GuardDamageEffect;

	/** 일반 공격을 막아냈을 때 가드 게이지에 쌓이는 양 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GuardBreak", meta = (ClampMin = "0.0"))
	float GuardBreakAmountPerHit = 20.f;

	/** 이 태그를 지닌 공격을 막으면 GuardBreakAmountPerHit 대신 GuardBreakHeavyAmount를 누적한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GuardBreak", meta = (Categories = "Shared.Attack.Weight"))
	FGameplayTag GuardBreakWeightTag;

	/** GuardBreakWeightTag가 실린 공격을 막아냈을 때 가드 게이지에 쌓이는 양 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GuardBreak", meta = (ClampMin = "0.0"))
	float GuardBreakHeavyAmount = 60.f;

	/** 가드가 무너질 때 재생할 몽타주. 비우면 가드 브레이크가 발생하지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GuardBreak")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	/**
	 * 가드가 무너질 때 적용할 GameplayEffect. 짧은 경직·재방어 금지 등을 여기서 구성한다.
	 * 체간 붕괴(PostureBroken)와는 다른 별개의 상태로 다루기 위해 GE를 별도로 지정한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GuardBreak")
	TSubclassOf<UGameplayEffect> GuardBreakEffect;

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

	/** Payload의 Instigator 방향으로 액터를 Yaw 회전시킨다 */
	void RotateActorToTargetFromEventData(const FGameplayEventData& Payload) const;

	/** 블록 히트 이벤트 기반으로 GameplayCue Parameters를 구성한다 */
	FGameplayCueParameters MakeBlockGameplayCueParams(const FGameplayEventData& Payload) const;

	void ExecuteSuccessfulBlockCue(const FGameplayEventData& Payload);
	void ExecuteParryCue(const FGameplayEventData& Payload);

	/** 패링 성공 시 카운터어택 윈도우 GameplayEffect를 적용한다 */
	void ApplyCounterAttackWindowEffect();

	/**
	 * @brief 이번에 막아낸 공격의 무게에 맞춰 GuardDamageEffect를 적용한다.
	 * 실제 누적·임계값 판정은 UACAttributeSet::HandleGuardDamage가 담당하며,
	 * 최대치 도달 시 Shared.Event.GuardBrokenTriggered 이벤트가 되돌아온다.
	 */
	void ApplyGuardDamage(const FGameplayEventData& Payload);

	/** Shared.Event.GuardBrokenTriggered 수신 — AttributeSet이 가드 붕괴를 알렸을 때 호출된다 */
	UFUNCTION()
	void OnGuardBrokenEventReceived(FGameplayEventData Payload);

	/**
	 * @brief 가드를 무너뜨린다 — GuardBreakEffect 적용 → 몽타주 재생 → Block 어빌리티 종료.
	 * 어빌리티가 끝나면서 Blocking 태그와 이동 제한 GE가 함께 해제되므로 실제로 방어가 풀린다.
	 */
	void TriggerGuardBreak();

	FGameplayEventData CachedPayload;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitGuardBrokenTask;
};
