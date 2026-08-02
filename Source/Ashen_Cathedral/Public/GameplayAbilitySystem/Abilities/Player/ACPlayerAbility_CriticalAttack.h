// 플레이어 크리티컬 어택(CriticalAttack) 어빌리티 — 체간 붕괴 상태의 Enemy를 정면에서 크리티컬 공격한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "Structs/ACStructTypes.h"
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
	/** 크리티컬 어택 몽타주 조합 목록 — 발동할 때마다 하나를 무작위로 골라 Player/Enemy가 동시에 재생한다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Animation")
	TArray<FACCriticalAttackMontagePair> CriticalAttackMontagePairs;

	/**
	 * 처형을 견디고 살아남은 Enemy가 피처형 몽타주에 이어서 재생할 기상 몽타주.
	 *
	 * 비워두면 재생하지 않고 지금까지처럼 곧바로 이동·AI를 복구한다.
	 * 조합에 상관없이 항상 같은 몽타주를 쓰므로, 시작 자세가 정확히 맞지 않는 것을 흡수하도록
	 * 이 몽타주의 Blend Mode In을 Inertialization으로 두는 것을 권한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Animation")
	TObjectPtr<UAnimMontage> EnemyGetUpMontage;

	/** 크리티컬 어택 시작 시 Player를 Enemy 전방에 배치할 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning", meta = (ClampMin = "50.0"))
	float CriticalAttackSnapOffset = 120.f;

	/** Motion Warping 타겟 이름 — 크리티컬 어택 몽타주의 MotionWarping NotifyState와 동일해야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning")
	FName WarpTargetName = "CriticalAttackTarget";

	/** 처형 마무리 이동 구간의 Motion Warping 타겟 이름 — 그 구간에 얹은 NotifyState와 동일해야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning")
	FName RetreatWarpTargetName = "CriticalAttackRetreatTarget";

	/** 처형 마무리 이동 타겟을 등록할지 여부. 마무리 구간에 MotionWarping NotifyState가 있는 몽타주에서만 켠다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning")
	bool bUseCriticalAttackEndWarp = true;

	/**
	 * 처형이 끝난 뒤 Player가 서 있을 지점을, Enemy 위치에서 Enemy 정면 축을 따라 잰 부호 있는 거리 (cm).
	 *
	 * 접근 지점이 +CriticalAttackSnapOffset 이므로 기준은 그 값이다.
	 *   - CriticalAttackSnapOffset보다 큰 양수 → Enemy 앞쪽으로 더 물러난다
	 *   - 음수 → Enemy를 지나쳐 뒤쪽에 선다 (스쳐 지나가는 연출)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning", meta = (EditCondition = "bUseCriticalAttackEndWarp"))
	float CriticalAttackEndOffset = 320.f;

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

	/**
	 * @brief CriticalAttackMontagePairs 중 Player/Enemy 몽타주가 모두 채워진 조합 하나를 무작위로 고른다.
	 *
	 * @return 선택된 조합의 인덱스, 유효한 조합이 하나도 없으면 INDEX_NONE
	 * @note 비어 있는 조합은 후보에서 빠지므로, 배열 중간에 미완성 항목이 있어도 나머지로 처형이 성립한다.
	 */
	int32 PickRandomMontagePairIndex() const;

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

	/**
	 * @brief EnemyGetUpMontage를 Enemy에게 재생한다.
	 *
	 * @return 재생을 시작했으면 true. 이 경우 이동·AI 복구를 미루고 기상 몽타주 종료 콜백에 맡겨야 한다.
	 * @note 몽타주가 지정되지 않았거나 재생에 실패하면 false를 반환하므로, 호출부는 기존처럼 즉시 복구하면 된다.
	 */
	bool TryPlayEnemyGetUpMontage(const AACEnemyCharacter* Enemy);

	/** Enemy의 이동 모드와 AI 로직을 복구한다 — 처형 연출이 완전히 끝난 시점에만 호출한다 */
	void RestoreEnemyAfterCriticalAttack(const AACEnemyCharacter* Enemy) const;

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

	/**
	 * @brief 피처형 몽타주의 블렌드아웃이 시작될 때 호출 — 여기서 기상 몽타주를 얹는다.
	 *
	 * @note OnMontageEnded는 블렌드아웃이 끝난 뒤에 오므로, 그때 기상을 재생하면 Idle로 돌아가
	 *       잠깐 선 자세를 거친 뒤 다시 눕는 그림이 된다. 블렌드아웃 시작 시점에 얹어야 두 블렌드가 겹친다.
	 */
	void OnEnemyMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> PlayerMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitDamageEventTask;

	TWeakObjectPtr<AACEnemyCharacter> CachedTargetEnemy;

	/** 이번 발동에서 선택된 몽타주 조합 — 종료 콜백이 Enemy 몽타주를 식별해야 하므로 EndAbility에서 리셋하지 않는다 */
	UPROPERTY()
	FACCriticalAttackMontagePair SelectedMontagePair;

	FTimerHandle HitStopTimerHandle;

	bool bCriticalAttackFinished = false;

	/** 기상 몽타주를 이미 얹었는지. 피처형 몽타주 종료가 이동·AI를 앞당겨 복구하는 것을 막는다 */
	bool bEnemyGetUpStarted = false;
};
