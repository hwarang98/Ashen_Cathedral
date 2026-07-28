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