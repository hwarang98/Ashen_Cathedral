// Boss(Enemy) 전용 Parry 어빌리티 — Startup 후 Shared.Status.Parry 윈도우를 부여하고, 성공/실패에 따라 반격 또는 후딜을 재생한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "ACEnemyAbility_Parry.generated.h"

class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;

UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_Parry : public UACEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UACEnemyAbility_Parry();

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
	/** 패링 자세로 진입하기까지의 선딜레이 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0"))
	float StartupDuration = 0.12f;

	/** Shared.Status.Parry가 유지되는 시간(초) — 실제 패링 판정 창. ACCalculation_DamageTaken이 이 태그를 검사한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0"))
	float ParryWindowDuration = 0.20f;

	/** 실패(타임아웃) 시 후딜 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0"))
	float FailureRecoveryDuration = 0.6f;

	/** 패링 자세 연출용 몽타주. 판정과는 무관하며 연출만 담당한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Animation")
	TObjectPtr<UAnimMontage> ParryMontage;

	/** 패링 성공 시 실행할 GameplayCue 태그. 기본값은 Player와 동일한 GameplayCue.FX.Parry (Enemy 전용 큐로 교체 가능) */
	UPROPERTY(EditDefaultsOnly, Category = "Parry|GameplayCue", meta = (Categories = "GameplayCue"))
	FGameplayTag SuccessfulParryCueTag;

	/**
	 * 패링 성공 시 TryActivateAbilitiesByTag로 실행할 카운터 공격 Ability의 AssetTag (예: Enemy.Ability.Parry.CounterAttack).
	 * 비어 있으면 카운터 Ability 실행 없이 기존 Parry 성공 처리만 수행한다.
	 * 실제 공격 판정/데미지는 이 태그가 가리키는 별도 공격 Ability가 담당한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Parry|CounterAttack", meta = (Categories = "Enemy.Ability"))
	FGameplayTag CounterAttackAbilityTag;

private:
	/** Startup 딜레이가 끝났을 때 호출 — ParryWindow를 연다 */
	UFUNCTION()
	void OnStartupDelayFinished();

	/** ParryWindow 시간이 다 되었을 때 호출 — 실패로 처리한다 */
	UFUNCTION()
	void OnParryWindowTimeout();

	/** Enemy.Event.ParrySuccess 수신 시 호출 — 성공으로 처리한다 */
	UFUNCTION()
	void OnParrySuccessEventReceived(FGameplayEventData Payload);

	/** 실패 후딜이 끝났을 때 호출 */
	UFUNCTION()
	void OnFailureRecoveryFinished();

	/**
	 * @brief Shared.Status.Parry 태그를 부여하고, 타임아웃과 성공 이벤트를 동시에 대기시킨다.
	 * 둘 중 먼저 도착하는 쪽이 ExitParryWindow를 호출해 판정을 종료시킨다.
	 */
	void EnterParryWindow();

	/**
	 * @brief Shared.Status.Parry 태그를 제거하고 성공/실패 여부에 따른 후속 처리를 진행한다.
	 * @param bSucceeded 패링 성공 여부
	 */
	void ExitParryWindow(bool bSucceeded);

	/** 판정과 무관하게 연출만 담당하는 몽타주를 재생한다 */
	void PlayCosmeticMontage(UAnimMontage* Montage);

	/** 패링 성공 이벤트 기반으로 GameplayCue Parameters를 구성한다 (UACPlayerAbility_Block::MakeBlockGameplayCueParams와 동일 패턴) */
	FGameplayCueParameters MakeParryGameplayCueParams(const FGameplayEventData& Payload) const;

	/** SuccessfulParryCueTag가 유효하면 패링 성공 GameplayCue를 실행한다. 비어 있으면 아무것도 하지 않는다. */
	void ExecuteSuccessfulParryCue(const FGameplayEventData& Payload);

	/**
	 * @brief CounterAttackAbilityTag와 AssetTag가 일치하는 Ability Spec을 찾아 활성화하고, 종료 감지를 위해 SpecHandle을 추적한다.
	 * 활성화에 성공하면 카운터 Ability가 끝날 때까지 이 Parry 어빌리티는 살아있어야 하므로 EndAbility를 호출하지 않는다.
	 * @return 카운터 Ability가 실제로 활성화되어 종료 대기 상태로 들어갔으면 true. 태그 미설정/ASC 없음/활성화 실패면 false.
	 * @note Shared.Status.Parry가 제거된 뒤(ExitParryWindow 내부) 호출되어야 카운터 Ability가 Parry 상태에 막히지 않는다.
	 */
	bool TryActivateCounterAttackAbility();

	/** ASC의 OnAbilityEnded 콜백 — 추적 중인 카운터 Ability(SpecHandle 일치)가 끝나면 Parry 어빌리티를 종료한다 */
	void OnCounterAttackAbilityEnded(const FAbilityEndedData& EndedData);

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> StartupDelayTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> WindowTimeoutTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> RecoveryDelayTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ParrySuccessTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	// 종료 대기 중인 카운터 공격 Ability의 SpecHandle. InstancedPerActor라 재활성화 간 값이 남지 않도록 EndAbility에서 초기화한다.
	FGameplayAbilitySpecHandle CounterAttackSpecHandle;

	// ASC OnAbilityEnded 바인딩 핸들. 외부 취소를 포함한 모든 종료 경로에서 EndAbility가 해제한다.
	FDelegateHandle CounterAttackEndedDelegateHandle;
};