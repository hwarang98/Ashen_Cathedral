// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/GameplayEffects/ACGameplayEffect_PostureCounter.h"
#include "ACGameplayTags.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"

UACGameplayEffect_PostureCounter::UACGameplayEffect_PostureCounter()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// SetByCaller로 전달된 체간 데미지를 PostureDamageTaken 메타 어트리뷰트에 가산한다.
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UACAttributeSet::GetPostureDamageTakenAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = ACGameplayTags::Shared_SetByCaller_PostureDamage;
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModifierInfo);
}
