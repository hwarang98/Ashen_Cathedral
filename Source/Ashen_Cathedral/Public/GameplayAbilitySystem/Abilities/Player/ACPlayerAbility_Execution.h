// 플레이어 처형(Execution) 어빌리티 — 그로기 상태의 Enemy를 정면에서 처형한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerAbility_Execution.generated.h"

class AACEnemyCharacter;
class UCameraShakeBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

/**
 * @brief 그로기 상태의 Enemy에게 처형 애니메이션을 재생하고 치명적인 데미지를 가하는 플레이어 전용 어빌리티.
 *
 * 좌클릭 입력 시 ACAbilitySystemComponent가 LightAttack보다 먼저 이 어빌리티를 TryActivate한다.
 * CanActivateAbility에서 처형 가능 대상이 없으면 false를 반환해 LightAttack이 정상 발동된다.
 *
 * 처형 흐름:
 *   1. FindExecutableTarget — Groggy·거리·정면 판정
 *   2. 적 상태 잠금 (Shared.Status.Executed 부여, Groggy 취소, 이동 잠금, BT 일시정지)
 *   3. 위치/회전 보정 (SnapToExecutionPoint)
 *   4. Player/Enemy 몽타주 동시 재생
 *   5. Shared.Event.ExecutionDamage AnimNotify 수신 시 데미지·CameraShake·HitStop 처리
 *   6. 몽타주 종료/취소 시 FinishExecution으로 상태 복구 후 EndAbility
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_Execution : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerAbility_Execution();

	/**
	 * @brief 처형 조건을 모두 만족하는 Enemy가 주변에 있을 때만 true를 반환한다.
	 *
	 * @note 조건: 플레이어 상태 정상 + 인접한 그로기 Enemy가 정면에 존재
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/**
	 * @brief 처형 시퀀스를 시작한다.
	 * 대상 잠금 -> 위치 보정 -> 몽타주 동시 재생 -> 데미지 이벤트 대기 순으로 진행된다.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 플레이어가 재생할 처형 몽타주 — AnimNotify_ExecutionDamage 포함 필요 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Animation")
	TObjectPtr<UAnimMontage> PlayerExecutionMontage;

	/** Enemy에게 재생할 처형당하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Animation")
	TObjectPtr<UAnimMontage> EnemyExecutedMontage;

	/** 처형 시작 시 Player를 Enemy 전방에 배치할 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Positioning", meta = (ClampMin = "50.0"))
	float ExecutionSnapOffset = 120.f;

	/** Motion Warping 타겟 이름 — 처형 몽타주의 MotionWarping NotifyState와 동일해야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Positioning")
	FName WarpTargetName = "ExecutionTarget";

	/** 처형 가능 최대 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Detection", meta = (ClampMin = "50.0"))
	float ExecutionDistance = 180.f;

	/**
	 * 처형 정면 판정 기준값.
	 * EnemyForward · normalize(PlayerPos - EnemyPos) >= 이 값이면 정면으로 인정.
	 * 0.65 ≈ ±49° 허용.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExecutionFrontDotThreshold = 0.65f;

	/** ExecutionDamageEffect에 SetByCaller로 전달할 처형 데미지 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Damage", meta = (ClampMin = "0.0"))
	float ExecutionDamage = 9999.f;

	/** 처형 데미지를 적용할 GameplayEffect — ACCalculation_DamageTaken을 통해 Enemy 처치 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Damage")
	TSubclassOf<UGameplayEffect> ExecutionDamageEffect;

	/** 타격 순간 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Camera")
	TSubclassOf<UCameraShakeBase> ExecutionCameraShakeClass;

	/** HitStop 지속 시간 (초) — Enemy의 CustomTimeDilation을 HitStopTimeDilation으로 낮춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopDuration = 0.08f;

	/** HitStop 중 Enemy CustomTimeDilation 값 (0에 가까울수록 더 느려짐) */
	UPROPERTY(EditDefaultsOnly, Category = "Execution|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.05f;

private:
	/**
	 * @brief 처형 가능한 가장 가까운 Enemy를 탐색한다.
	 *
	 * 조건: Shared.Status.Groggy 보유 + Dead/Executed/Phase2/Invincible 미보유 +
	 *       ExecutionDistance 이내 + EnemyForward·(PlayerPos-EnemyPos) >= ExecutionFrontDotThreshold
	 *
	 * @param InPlayer 탐색 기준 플레이어 캐릭터
	 * @return 처형 가능한 Enemy, 없으면 nullptr
	 */
	AACEnemyCharacter* FindExecutableTarget(const AACPlayerCharacter* InPlayer) const;

	/** Enemy에 Shared.Status.Executed를 부여하고 Groggy 취소 -> 이동 잠금 -> BT 일시정지를 수행한다. */
	void LockEnemyForExecution(const AACEnemyCharacter* Enemy) const;

	/**
	 * @brief MotionWarpingComponent에 처형 Warp 타겟을 등록하고 Enemy를 Player 방향으로 회전시킨다.
	 * Player의 실제 이동은 루트 모션 + MotionWarping이 처리한다.
	 * @note 처형 몽타주에 WarpTargetName과 일치하는 AnimNotifyState_MotionWarping이 있어야 한다.
	 */
	void SetupExecutionMotionWarp(const AACPlayerCharacter* Player, AACEnemyCharacter* Enemy) const;

	/** 처형 종료 시 Enemy 상태를 복구한다. 사망 상태면 Death 어빌리티에 위임하고 즉시 반환한다. */
	void UnlockEnemy(const AACEnemyCharacter* Enemy);

	/** 처형 종료 공통 처리 — 태스크 정리 → UnlockEnemy → EndAbility */
	void FinishExecution(bool bWasCancelled);

	/** 플레이어 몽타주 정상 종료(Completed/BlendOut) 시 호출 */
	UFUNCTION()
	void OnPlayerMontageCompleted();

	/** 플레이어 몽타주 취소/중단 시 호출 */
	UFUNCTION()
	void OnPlayerMontageCancelled();

	/**
	 * @brief Shared.Event.ExecutionDamage 이벤트 수신 시 호출.
	 * ExecutionDamageEffect 적용, CameraShake, HitStop을 처리한다.
	 * AnimNotify가 Player 몽타주의 타격 타이밍에 이 이벤트를 발송해야 한다.
	 */
	UFUNCTION()
	void OnExecutionDamageEventReceived(FGameplayEventData Payload);

	/** HitStop 타이머 만료 시 Enemy의 CustomTimeDilation을 1.0으로 복원한다. */
	UFUNCTION()
	void RestoreEnemyTimeDilation();

	/** Enemy 처형 몽타주 종료 시 호출 — 이동 복구 및 BT 재개를 처리한다. */
	UFUNCTION()
	void OnEnemyMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> PlayerMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitDamageEventTask;

	TWeakObjectPtr<AACEnemyCharacter> CachedTargetEnemy;

	FTimerHandle HitStopTimerHandle;

	bool bExecutionFinished = false;
};