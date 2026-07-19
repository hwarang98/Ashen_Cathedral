// Boss(Enemy) 전용 Block 어빌리티 — BlockMontage 재생과 Enemy.Status.Blocking 부여를 담당한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACGameplayAbility_Block.h"
#include "ACEnemyAbility_Block.generated.h"

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
	/** Enemy가 Block 성공 시 Block 상태(어빌리티/Blocking 태그)를 유지한 채 재생할 짧은 반응 몽타주. None이면 자세 몽타주를 그대로 유지한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Reaction")
	TObjectPtr<UAnimMontage> BlockHitMontage;

	/** Block 성공 시 Enemy ASC에서 실행할 GameplayCue 태그. 기본값은 Player와 동일한 GameplayCue.FX.SuccessfulBlock (Enemy 전용 큐로 교체 가능) */
	UPROPERTY(EditDefaultsOnly, Category = "Block|GameplayCue", meta = (Categories = "GameplayCue"))
	FGameplayTag SuccessfulBlockCueTag;

private:
	/** Block 자세(유지) 몽타주가 정상 종료(완료/블렌드아웃)됐을 때 호출 — Block 유지 시간이 끝난 것이므로 어빌리티를 종료한다 */
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

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitBlockEventTask;
};
