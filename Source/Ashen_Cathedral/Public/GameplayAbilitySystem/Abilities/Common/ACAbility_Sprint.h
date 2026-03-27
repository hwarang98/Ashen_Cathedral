// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACAbility_Sprint.generated.h"

/**
 * UACAbility_Sprint 클래스는 Sprint 기능을 구현한 능력(Ability)이다.
 * 캐릭터의 이동 속도를 증가시키고, 스태미나 소모와 같은 추가적인 게임플레이 효과를 제공한다.
 * - Input Press -> ActivateAbility (SprintSpeed 값으로 Dynamic GE 생성·적용 → MoveSpeed 어트리뷰트 Override → MaxWalkSpeed 자동 동기화, 드레인 GE 적용)
 * - Input Release -> EndAbility (SprintSpeedEffectClass GE 제거 → MoveSpeed 복원 → MaxWalkSpeed 자동 동기화, 드레인 GE 제거)
 * - Stamina 소진 -> EndAbility 자동 호출
 * - AnimInstance: ActivationOwnedTags 에 Shared.Status.Sprinting 부여 -> IsSprinting 갱신
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACAbility_Sprint : public UACGameplayAbility
{
	GENERATED_BODY()

public:
	UACAbility_Sprint();

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

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

	/** 활성 중 재입력 시 Sprint 종료 (토글) */
	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

private:
	/** Sprint 중 적용할 MoveSpeed Override 값. 런타임 Dynamic GE 로 어트리뷰트에 전달. */
	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	float SprintSpeed = 500.f;

	/** 해당 속도(XY 평면) 이하에서는 Sprint 진입 불가 */
	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	float SprintStopThreshold = 50.f;

	/**
	 * Infinite Periodic 스태미나 드레인 GE.
	 * 소비량, 주기는 GE 에셋에서 설정.
	 * 적 StartupData 에서는 nullptr 로 두면 소비 없음.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainEffectClass;

	FActiveGameplayEffectHandle SprintSpeedEffectHandle;
	FActiveGameplayEffectHandle DrainEffectHandle;
	FDelegateHandle StaminaDelegateHandle;

	void OnStaminaChanged(const FOnAttributeChangeData& Data);
};