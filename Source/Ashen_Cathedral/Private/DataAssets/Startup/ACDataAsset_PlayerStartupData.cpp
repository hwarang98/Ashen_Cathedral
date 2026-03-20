// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/Startup/ACDataAsset_PlayerStartupData.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"

void UACDataAsset_PlayerStartupData::GiveToAbilitySystemComponent(UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	Super::GiveToAbilitySystemComponent(InASCToGive, ApplyLevel);

	for (const FACPlayerAbilitySet& AbilitySet : PlayerStartUpAbilitySets)
	{
		if (!AbilitySet.IsValid())
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
		AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;

		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);

		InASCToGive->GiveAbility(AbilitySpec);
	}

	// 스태미나 회복 딜레이 GE 레퍼런스를 ASC에 전달 (AttributeSet의 런타임 접근용)
	InASCToGive->StaminaRegenDelayEffectClass = StaminaRegenDelayEffectClass;
}