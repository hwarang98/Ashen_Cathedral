// 체간이 붕괴한 플레이어를 처형하는 Enemy 전용 크리티컬 어택 어빌리티 — 플레이어의 CriticalAttack을 반대 방향으로 미러링한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "ACEnemyAbility_CriticalAttack.generated.h"

class AACPlayerCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UCameraShakeBase;

/** Enemy가 재생할 크리티컬 몽타주와, 같은 순간 플레이어가 재생할 피격 몽타주의 쌍 */
USTRUCT(BlueprintType)
struct FACEnemyCriticalAttackMontagePair
{
	GENERATED_BODY()

	/** Enemy가 재생할 크리티컬 공격 몽타주. 타격 타이밍에 Shared.Event.CriticalAttackDamage를 발송하는 AnimNotify가 필요하다 */
	UPROPERTY(EditAnywhere, Category = "CriticalAttack")
	TObjectPtr<UAnimMontage> EnemyMontage;

	/** 위 몽타주와 맞물려 플레이어가 재생할 피격 몽타주 */
	UPROPERTY(EditAnywhere, Category = "CriticalAttack")
	TObjectPtr<UAnimMontage> PlayerVictimMontage;
};

/**
 * @brief 체간 붕괴(Shared.Status.PostureBroken) 상태의 플레이어에게 크리티컬 공격을 실행하는 Enemy 어빌리티.
 *
 * 흐름:
 *   1. FindCriticalAttackTarget — 체간 붕괴·거리·정면 판정으로 대상 플레이어를 찾는다
 *   2. LockPlayerForCriticalAttack — Shared.Status.Executed 부여, 체간 붕괴 어빌리티 취소, 이동/입력 잠금
 *   3. 몽타주 쌍을 랜덤으로 하나 뽑아 Enemy/플레이어에 동시 재생
 *   4. Shared.Event.CriticalAttackDamage 수신 시 데미지와 카메라 셰이크 적용
 *   5. 몽타주 종료/취소 시 플레이어 상태를 복구하고 EndAbility
 *
 * @note 몽타주는 쌍 단위로 뽑는다. Enemy와 플레이어 애니메이션이 서로 맞물려야 연출이 성립하므로 개별 랜덤은 쓰지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_CriticalAttack : public UACEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UACEnemyAbility_CriticalAttack();

	/** 체간 붕괴한 플레이어가 사거리 정면에 있을 때만 true를 반환한다 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/**
	 * @brief 크리티컬 공격 시퀀스를 시작한다.
	 * 대상 잠금 → 위치 보정 → 몽타주 쌍 동시 재생 → 데미지 이벤트 대기 순으로 진행된다.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 실행할 크리티컬 연출 목록. 활성화할 때마다 이 중 하나를 무작위로 고른다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Animation")
	TArray<FACEnemyCriticalAttackMontagePair> CriticalAttackMontages;

	/**
	 * 크리티컬을 견디고 살아남은 플레이어가 피격 몽타주에 이어서 재생할 기상 몽타주.
	 *
	 * 비워두면 재생하지 않고 지금까지처럼 곧바로 이동·입력을 복구한다.
	 * 조합에 상관없이 항상 같은 몽타주를 쓰므로, 시작 자세가 정확히 맞지 않는 것을 흡수하도록
	 * 이 몽타주의 Blend Mode In을 Inertialization으로 두는 것을 권한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Animation")
	TObjectPtr<UAnimMontage> PlayerGetUpMontage;

	/** 크리티컬 공격 가능 최대 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Detection", meta = (ClampMin = "50.0"))
	float CriticalAttackDistance = 200.f;

	/**
	 * 정면 판정 기준값.
	 * EnemyForward · normalize(PlayerPos - EnemyPos) >= 이 값이면 정면으로 인정한다.
	 * 0.65 ≈ ±49°
	 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalAttackFrontDotThreshold = 0.65f;

	/** 연출 시작 시 Enemy가 도달할 플레이어 전방 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning", meta = (ClampMin = "50.0"))
	float CriticalAttackSnapOffset = 130.f;

	/** Motion Warping 타겟 이름 — Enemy 크리티컬 몽타주의 MotionWarping NotifyState와 동일해야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Positioning")
	FName WarpTargetName = "CriticalAttackTarget";

	/** CriticalAttackDamageEffect에 SetByCaller로 전달할 데미지 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Damage", meta = (ClampMin = "0.0"))
	float CriticalAttackDamage = 100.f;

	/** 데미지를 적용할 GameplayEffect — ACCalculation_DamageTaken을 통해 처리된다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Damage")
	TSubclassOf<UGameplayEffect> CriticalAttackDamageEffect;

	/** 타격 순간 플레이어에게 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Camera")
	TSubclassOf<UCameraShakeBase> CriticalAttackCameraShakeClass;

	/** true면 사거리·정면 판정각·워프 목표 지점을 화면에 그린다. 판정 결과는 초록(통과)/빨강(탈락)으로 구분된다 */
	UPROPERTY(EditDefaultsOnly, Category = "CriticalAttack|Debug")
	bool bDebugDrawDetection = false;

private:
	/**
	 * @brief 크리티컬 공격이 가능한 플레이어를 탐색한다.
	 *
	 * 조건: Shared.Status.PostureBroken 보유 + Dead/Executed/Invincible 미보유 +
	 *       CriticalAttackDistance 이내 + 정면 판정 통과
	 *
	 * @return 조건을 만족하는 플레이어, 없으면 nullptr
	 */
	AACPlayerCharacter* FindCriticalAttackTarget() const;

	/** 상태(체간 붕괴/사망/무적)·거리·정면 조건을 모두 만족하는지 검사한다 */
	bool IsValidCriticalAttackTarget(const AACEnemyCharacter* EnemyCharacter, const AACPlayerCharacter* Player) const;

	/** bDebugDrawDetection이 켜져 있을 때 사거리 구·정면 판정 원뿔·판정 결과를 그린다 */
	void DrawDetectionDebug(const AACEnemyCharacter* EnemyCharacter, const AActor* TargetPlayer, bool bAccepted) const;

	/** CriticalAttackMontages에서 양쪽 몽타주가 모두 유효한 항목만 후보로 삼아 하나를 무작위로 고른다 */
	const FACEnemyCriticalAttackMontagePair* PickMontagePair() const;

	/** 플레이어에 Shared.Status.Executed를 부여하고 체간 붕괴 어빌리티 취소 → 이동/입력 잠금을 수행한다 */
	void LockPlayerForCriticalAttack(AACPlayerCharacter* Player) const;

	/**
	 * @brief Enemy의 MotionWarping 타겟을 플레이어 전방에 등록하고, 피격자인 플레이어는 즉시 마주보게 회전시킨다.
	 * Enemy의 실제 이동은 크리티컬 몽타주의 루트 모션 + MotionWarping이 담당한다.
	 * @note 몽타주에 WarpTargetName과 일치하는 AnimNotifyState_MotionWarping이 없으면 Enemy는 제자리에서 연출한다.
	 */
	void SetupCriticalAttackMotionWarp(AACPlayerCharacter* Player) const;

	/** 잠갔던 플레이어의 이동/입력을 복구하고 Executed 태그를 제거한다. 피격·기상 몽타주가 남아 있으면 복구를 콜백으로 넘긴다 */
	void UnlockPlayer(AACPlayerCharacter* Player) const;

	/**
	 * @brief PlayerGetUpMontage를 플레이어에게 재생한다.
	 *
	 * @return 재생을 시작했으면 true. 이 경우 이동·입력 복구를 미루고 기상 몽타주 종료 콜백에 맡겨야 한다.
	 * @note 몽타주가 지정되지 않았거나 재생에 실패하면 false를 반환하므로, 호출부는 기존처럼 즉시 복구하면 된다.
	 */
	bool TryPlayPlayerGetUpMontage(AACPlayerCharacter* Player) const;

	/** 플레이어의 이동 모드와 입력을 복구한다 — 처형 연출이 완전히 끝난 시점에만 호출한다 */
	void RestorePlayerAfterCriticalAttack(AACPlayerCharacter* Player) const;

	/** 종료 공통 처리 — UnlockPlayer 후 EndAbility. 중복 호출은 무시된다 */
	void FinishCriticalAttack(bool bWasCancelled);

	/** Enemy 몽타주 정상 종료 시 호출 */
	UFUNCTION()
	void OnEnemyMontageCompleted();

	/** Enemy 몽타주 취소/중단 시 호출 */
	UFUNCTION()
	void OnEnemyMontageCancelled();

	/** Shared.Event.CriticalAttackDamage 수신 시 호출 — 데미지와 카메라 셰이크를 적용한다 */
	UFUNCTION()
	void OnCriticalAttackDamageEventReceived(FGameplayEventData Payload);

	/** 플레이어 피격·기상 몽타주 종료 시 호출 — 연출이 완전히 끝났으면 이동/입력을 복구한다 */
	UFUNCTION()
	void OnPlayerVictimMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/**
	 * @brief 플레이어 피격 몽타주의 블렌드아웃이 시작될 때 호출 — 여기서 기상 몽타주를 얹는다.
	 *
	 * @note OnMontageEnded는 블렌드아웃이 끝난 뒤에 오므로, 그때 기상을 재생하면 Idle로 돌아가
	 *       잠깐 선 자세를 거친 뒤 다시 눕는 그림이 된다. 블렌드아웃 시작 시점에 얹어야 두 블렌드가 겹친다.
	 */
	void OnPlayerVictimMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> EnemyMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitDamageEventTask;

	TWeakObjectPtr<AACPlayerCharacter> CachedTargetPlayer;

	// 플레이어에게 재생 중인 피격 몽타주. 취소 시 정리 대상 판별에 쓴다
	UPROPERTY()
	TObjectPtr<UAnimMontage> ActivePlayerVictimMontage;

	bool bCriticalAttackFinished = false;

	/** 기상 몽타주를 이미 얹었는지. 피격 몽타주 종료가 이동·입력을 앞당겨 복구하는 것을 막는다 */
	bool bPlayerGetUpStarted = false;
};
