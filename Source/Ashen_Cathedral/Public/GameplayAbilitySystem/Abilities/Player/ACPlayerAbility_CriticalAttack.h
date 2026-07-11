// 플레이어 크리티컬 어택(CriticalAttack) 어빌리티 — 체간 붕괴 상태의 Enemy를 정면에서 크리티컬 공격한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerAbility_CriticalAttack.generated.h"

class AACEnemyCharacter;
class UCameraShakeBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

/**
 * @brief 체간 붕괴 상태의 Enemy에게 크리티컬 어택 애니메이션을 재생하고 데미지를 가하는 플레이어 전용 어빌리티.
 *
 * 좌클릭 입력 시 ACAbilitySystemComponent가 LightAttack보다 먼저 이 어빌리티를 TryActivate한다.
 * CanActivateAbility에서 크리티컬 어택 가능 대상이 없으면 false를 반환해 LightAttack이 정상 발동된다.
 *
 * 크리티컬 어택 흐름:
 *   1. FindCriticalAttackTarget — 체간 붕괴·거리·정면 판정
 *   2. 적 상태 잠금 (Shared.Status.Executed 부여, 체간 붕괴 취소, 이동 잠금, BT 일시정지)
 *   3. 위치/회전 보정 (SnapToExecutionPoint)
 *   4. Player/Enemy 몽타주 동시 재생
 *   5. Shared.Event.CriticalAttackDamage AnimNotify 수신 시 데미지·CameraShake·HitStop 처리
 *   6. 몽타주 종료/취소 시 FinishCriticalAttack으로 상태 복구 후 EndAbility
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_CriticalAttack : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerAbility_CriticalAttack();

	/**
	 * @brief 크리티컬 어택 조건을 모두 만족하는 Enemy가 주변에 있을 때만 true를 반환한다.
	 *
	 * @note 조건: 플레이어 상태 정상 + 인접한 체간 붕괴 Enemy가 정면에 존재
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/**
	 * @brief 크리티컬 어택 시퀀스를 시작한다.
	 * 대상 잠금 -> 위치 보정 -> 몽타주 동시 재생 -> 데미지 이벤트 대기 순으로 진행된다.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 플레이어가 재생할 크리티컬 어택 몽타주 — AnimNotify_CriticalAttackDamage 포함 필요 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Animation")
	TObjectPtr<UAnimMontage> PlayerCriticalAttackMontage;

	/** Enemy에게 재생할 크리티컬 어택당하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Animation")
	TObjectPtr<UAnimMontage> EnemyCriticalAttackedMontage;

	/** 크리티컬 어택 시작 시 Player를 Enemy 전방에 배치할 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning", meta = (ClampMin = "50.0"))
	float CriticalAttackSnapOffset = 120.f;

	/** Motion Warping 타겟 이름 — 크리티컬 어택 몽타주의 MotionWarping NotifyState와 동일해야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning")
	FName WarpTargetName = "CriticalAttackTarget";

	/** 크리티컬 어택 가능 최대 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Detection", meta = (ClampMin = "50.0"))
	float CriticalAttackDistance = 180.f;

	/**
	 * 크리티컬 어택 정면 판정 기준값.
	 * EnemyForward · normalize(PlayerPos - EnemyPos) >= 이 값이면 정면으로 인정.
	 * 0.65 ≈ ±49° 허용.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalAttackFrontDotThreshold = 0.65f;

	/** CriticalAttackDamageEffect에 SetByCaller로 전달할 크리티컬 어택 데미지 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Damage", meta = (ClampMin = "0.0"))
	float CriticalAttackDamage = 9999.f;

	/** 크리티컬 어택 데미지를 적용할 GameplayEffect — ACCalculation_DamageTaken을 통해 Enemy 처치 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Damage")
	TSubclassOf<UGameplayEffect> CriticalAttackDamageEffect;

	/** 타격 순간 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Camera")
	TSubclassOf<UCameraShakeBase> CriticalAttackCameraShakeClass;

	/** HitStop 지속 시간 (초) — Enemy의 CustomTimeDilation을 HitStopTimeDilation으로 낮춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopDuration = 0.08f;

	/** HitStop 중 Enemy CustomTimeDilation 값 (0에 가까울수록 더 느려짐) */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.05f;

private:
	/**
	 * @brief 크리티컬 어택 가능한 가장 가까운 Enemy를 탐색한다.
	 *
	 * 조건: Shared.Status.PostureBroken 보유 + Dead/Executed/Phase2/Invincible 미보유 +
	 *       CriticalAttackDistance 이내 + EnemyForward·(PlayerPos-EnemyPos) >= CriticalAttackFrontDotThreshold
	 *
	 * @param InPlayer 탐색 기준 플레이어 캐릭터
	 * @return 크리티컬 어택 가능한 Enemy, 없으면 nullptr
	 */
	AACEnemyCharacter* FindCriticalAttackTarget(const AACPlayerCharacter* InPlayer) const;

	/** Enemy에 Shared.Status.Executed를 부여하고 체간 붕괴 어빌리티 취소 -> 이동 잠금 -> BT 일시정지를 수행한다. */
	void LockEnemyForCriticalAttack(const AACEnemyCharacter* Enemy) const;

	/**
	 * @brief MotionWarpingComponent에 크리티컬 어택 Warp 타겟을 등록하고 Enemy를 Player 방향으로 회전시킨다.
	 * Player의 실제 이동은 루트 모션 + MotionWarping이 처리한다.
	 * @note 크리티컬 어택 몽타주에 WarpTargetName과 일치하는 AnimNotifyState_MotionWarping이 있어야 한다.
	 */
	void SetupCriticalAttackMotionWarp(const AACPlayerCharacter* Player, AACEnemyCharacter* Enemy) const;

	/** 크리티컬 어택 종료 시 Enemy 상태를 복구한다. 사망 상태면 Death 어빌리티에 위임하고 즉시 반환한다. */
	void UnlockEnemy(const AACEnemyCharacter* Enemy);

	/** 크리티컬 어택 종료 공통 처리 — 태스크 정리 → UnlockEnemy → EndAbility */
	void FinishCriticalAttack(bool bWasCancelled);

	/** 플레이어 몽타주 정상 종료(Completed/BlendOut) 시 호출 */
	UFUNCTION()
	void OnPlayerMontageCompleted();

	/** 플레이어 몽타주 취소/중단 시 호출 */
	UFUNCTION()
	void OnPlayerMontageCancelled();

	/**
	 * @brief Shared.Event.CriticalAttackDamage 이벤트 수신 시 호출.
	 * CriticalAttackDamageEffect 적용, CameraShake, HitStop을 처리한다.
	 * AnimNotify가 Player 몽타주의 타격 타이밍에 이 이벤트를 발송해야 한다.
	 */
	UFUNCTION()
	void OnCriticalAttackDamageEventReceived(FGameplayEventData Payload);

	/** HitStop 타이머 만료 시 Enemy의 CustomTimeDilation을 1.0으로 복원한다. */
	UFUNCTION()
	void RestoreEnemyTimeDilation();

	/** Enemy 크리티컬 어택 몽타주 종료 시 호출 — 이동 복구 및 BT 재개를 처리한다. */
	UFUNCTION()
	void OnEnemyMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> PlayerMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitDamageEventTask;

	TWeakObjectPtr<AACEnemyCharacter> CachedTargetEnemy;

	FTimerHandle HitStopTimerHandle;

	bool bCriticalAttackFinished = false;
};
