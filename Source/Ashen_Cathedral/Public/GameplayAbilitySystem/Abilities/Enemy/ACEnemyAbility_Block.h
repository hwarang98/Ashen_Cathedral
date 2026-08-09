// Boss(Enemy) 전용 Block 어빌리티 — BlockMontage 재생과 Enemy.Status.Blocking 부여를 담당한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACGameplayAbility_Block.h"
#include "ACEnemyAbility_Block.generated.h"

class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_Block : public UACGameplayAbility_Block
{
	GENERATED_BODY()

public:
	UACEnemyAbility_Block();

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
	/** Block Ability가 정상적으로 유지되는 시간. 몽타주 길이와 무관하게 이 시간이 끝나면 방어를 종료한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Timing", meta = (ClampMin = "0.1", Units = "s"))
	float BlockDuration = 2.5f;

	/** Enemy가 Block 성공 시 Block 상태(어빌리티/Blocking 태그)를 유지한 채 재생할 짧은 반응 몽타주. None이면 자세 몽타주를 그대로 유지한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Reaction")
	TObjectPtr<UAnimMontage> BlockHitMontage;

	/** Block 성공 시 Enemy ASC에서 실행할 GameplayCue 태그. 기본값은 Player와 동일한 GameplayCue.FX.SuccessfulBlock (Enemy 전용 큐로 교체 가능) */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GameplayCue", meta = (Categories = "GameplayCue"))
	FGameplayTag SuccessfulBlockCueTag;

	/**
	 * 저스트 가드(패링) 판정 창은 이 어빌리티가 아니라 BlockMontage에 배치한 ANS_AddGameplayTag(Shared.Status.Parry)
	 * 노티파이가 관리한다(플레이어 블록과 동일 방식). 창 안에 피격되면 ACCalculation_DamageTaken이 패링(데미지 0 +
	 * 체간 역공 + Stagger)으로 처리하고 Enemy.Event.ParrySuccess를 발송한다. 이 어빌리티는 그 이벤트를 받아 카운터만 실행한다.
	 * 몽타주에 노티파이가 없는 기존 보스 Block은 Shared.Status.Parry가 부여되지 않아 순수 블록으로 동작한다.
	 */

	/** 저스트 가드(패링) 성공 시 실행할 카운터 공격 Ability의 AssetTag. 비어 있으면 카운터 없이 패링 판정만 적용한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Parry", meta = (Categories = "Enemy.Ability"))
	FGameplayTag ParryCounterAttackAbilityTag;

	/** 저스트 가드(패링) 성공 시 실행할 GameplayCue 태그. 비어 있으면 큐를 실행하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Parry", meta = (Categories = "GameplayCue"))
	FGameplayTag SuccessfulParryCueTag;

private:
	/** BlockDuration이 끝나면 방어 Ability를 정상 종료한다. */
	UFUNCTION()
	void OnBlockDurationFinished();

	/** Block 자세 몽타주가 정상 종료됐을 때 호출 — 남은 BlockDuration 동안 자세 몽타주를 다시 재생한다. */
	UFUNCTION()
	void OnMontageCompleted();

	/** 몽타주가 외부 요인(그로기/사망 몽타주 등)으로 강제 중단됐을 때 호출 — 진짜 종료 조건이므로 어빌리티를 취소 종료한다 */
	UFUNCTION()
	void OnMontageCancelled();

	/** BlockHit(움찔) 몽타주가 정상 종료됐을 때 호출 — 다시 Block 자세 몽타주로 복귀한다 */
	UFUNCTION()
	void OnBlockHitMontageFinished();

	/**
	 * @brief Player.Event.SuccessfulBlock 수신 시 호출 (TryTriggerSuccessfulBlockEvent가 방어자에게 보내는 이벤트).
	 * Block 성공 GameplayCue를 실행하고, BlockHitMontage가 설정되어 있으면 Block 상태를 유지한 채 재생한다.
	 * 이 몽타주 전환은 어빌리티 종료로 이어지지 않는다.
	 */
	UFUNCTION()
	void OnSuccessfulBlockEventReceived(FGameplayEventData Payload);

	/**
	 * @brief Block 자세(유지) 몽타주를 재생한다.
	 * 기존 몽타주 태스크가 있으면 조용히 종료(EndTask)해, 의도적 몽타주 전환이
	 * OnInterrupted 콜백을 거쳐 EndAbility로 이어지지 않게 한다.
	 */
	void PlayHoldMontage();

	/** Enemy.Event.ParrySuccess 수신 — 저스트 가드 성공 시 패링 큐를 실행하고 카운터 공격을 시도한다 */
	UFUNCTION()
	void OnParrySuccessEventReceived(FGameplayEventData Payload);

	/** SuccessfulParryCueTag가 유효하면 패링 성공 GameplayCue를 실행한다 */
	void ExecuteSuccessfulParryCue(const FGameplayEventData& Payload);

	/**
	 * @brief ParryCounterAttackAbilityTag와 AssetTag가 일치하는 Ability를 찾아 활성화하고 종료를 추적한다.
	 * 카운터가 끝날 때까지 StateTree Block 상태를 유지시키기 위해 이 Block 어빌리티는 살려두며(EndAbility 미호출),
	 * 종료는 OnParryCounterAttackEnded가 담당한다.
	 * @return 카운터가 실제로 활성화되어 종료 대기에 들어갔으면 true.
	 */
	bool TryActivateParryCounterAttack();

	/** ASC의 OnAbilityEnded 콜백 — 추적 중인 카운터 공격(SpecHandle 일치)이 끝나면 Block 어빌리티를 종료한다 */
	void OnParryCounterAttackEnded(const FAbilityEndedData& EndedData);

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitBlockEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ParrySuccessTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> BlockDurationTask;

	// 종료 대기 중인 카운터 공격 Ability의 SpecHandle. InstancedPerActor라 재활성화 간 값이 남지 않도록 EndAbility에서 초기화한다.
	FGameplayAbilitySpecHandle ParryCounterAttackSpecHandle;

	// ASC OnAbilityEnded 바인딩 핸들. 모든 종료 경로에서 EndAbility가 해제한다.
	FDelegateHandle ParryCounterAttackEndedHandle;
};
