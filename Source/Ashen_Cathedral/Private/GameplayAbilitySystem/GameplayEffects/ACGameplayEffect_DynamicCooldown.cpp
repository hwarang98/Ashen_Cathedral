// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/GameplayEffects/ACGameplayEffect_DynamicCooldown.h"
#include "GameplayTags/ACGameplayTags_Shared.h"

UACGameplayEffect_DynamicCooldown::UACGameplayEffect_DynamicCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCallerInfo;
	SetByCallerInfo.DataTag = ACGameplayTags::Shared_SetByCaller_CooldownDuration;

	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCallerInfo);
}
