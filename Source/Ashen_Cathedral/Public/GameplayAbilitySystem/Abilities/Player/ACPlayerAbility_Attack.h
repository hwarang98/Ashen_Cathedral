// 플레이어 전용 공격 어빌리티 — 스태미나 조건 확인, 콤보 리셋 타이머, 즉시 콤보 체인 처리

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACPlayerAbility_Attack.generated.h"

class UACAbilitySystemComponent;

/**
 * 플레이어 전용 공격 어빌리티.
 * 몽타주가 자연 완료되면 콤보 리셋 타이머를 시작하고, 플레이어가 공격 중 재입력하면
 * TriggerComboChain()을 통해 현재 몽타주를 즉시 중단하고 다음 콤보로 전환한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_Attack : public UACAbility_Attack
{
	GENERATED_BODY()

public:
	UACPlayerAbility_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 스태미나가 0 이하면 공격을 차단한다. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/** 몽타주 자연 완료 후 콤보 리셋 타이머를 시작한다. */
	virtual void HandleComboComplete() override;

	/**
	 * @brief 콤보 취소 처리.
	 * bComboChaining 플래그가 true이면 ResetComboCount를 건너뛴다.
	 * TriggerComboChain()이 EndAbility(true) 직전에 이 플래그를 설정한다.
	 */
	virtual void HandleComboCancelled() override;

	/**
	 * @brief 어빌리티가 활성 중일 때 즉시 콤보 체인을 트리거한다.
	 * EndAbility(true) 완료 직후 같은 프레임 안에서 다음 어빌리티를 동기 활성화한다.
	 * Input_AbilityInputPressed에서 LightAttack이 들어왔을 때 외부 호출용.
	 */
	void TriggerComboChain();

private:
	/** 콤보 완료 후 이 시간 안에 재입력이 없으면 콤보 카운트가 리셋된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (AllowPrivateAccess = "true"))
	float ComboResetDelay = 2.0f;

	FTimerHandle ComboResetTimerHandle;

	void OnComboResetTimerExpired();

	/**
	 * TriggerComboChain()이 EndAbility 직전에 true로 설정한다.
	 * HandleComboCancelled에서 ResetComboCount를 건너뛰는 용도로만 사용한다.
	 */
	bool bComboChaining = false;
};
