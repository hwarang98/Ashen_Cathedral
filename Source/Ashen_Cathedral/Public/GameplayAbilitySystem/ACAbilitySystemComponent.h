// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ACAbilitySystemComponent.generated.h"

/**
 *
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void OnAbilityInputPressed(const FGameplayTag& InputTag);
	void OnAbilityInputReleased(const FGameplayTag& InputTag);

	/**
	 * @brief InputTag를 Dynamic Spec Source Tag로 가진 어빌리티 Spec 중 현재 활성 중이 아닌 것을 찾는다.
	 *
	 * @param InputTag 어빌리티를 부여할 때 Dynamic Spec Source Tag로 넣어둔 입력 태그
	 * @return 조건을 만족하는 첫 번째 Spec. 없으면 nullptr
	 * @note 반환 포인터는 ActivatableAbilities 배열의 원소를 직접 가리키므로 어빌리티 부여/제거가 일어나면
	 *       무효화된다. 받은 즉시 Handle을 복사해 쓰는 용도로만 사용한다.
	 */
	FGameplayAbilitySpec* FindInactiveAbilitySpecByInputTag(const FGameplayTag& InputTag);

	/** DA_PlayerStartup에서 GiveToAbilitySystemComponent 시점에 주입됩니다. 적은 nullptr. */
	UPROPERTY()
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffectClass;

	/** DA_StartupData(Base)에서 GiveToAbilitySystemComponent 시점에 주입됩니다. Posture 피해 시 재적용되어 자연 감소 시작을 지연시킵니다. */
	UPROPERTY()
	TSubclassOf<UGameplayEffect> PostureDecayDelayEffectClass;

	/** DA_StartupData(Base)에서 주입됩니다. 가드로 막아낼 때마다 재적용되어 GuardGauge 자연 감소 시작을 지연시킵니다. */
	UPROPERTY()
	TSubclassOf<UGameplayEffect> GuardDecayDelayEffectClass;
};