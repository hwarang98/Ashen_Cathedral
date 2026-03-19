// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/Startup/ACDataAsset_StartupDataBase.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

void UACDataAsset_StartupDataBase::GiveToAbilitySystemComponent(UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	GrantAbilities(ActivateOnGivenAbilities, InASCToGive, ApplyLevel);
	GrantAbilities(ReactiveAbilities, InASCToGive, ApplyLevel);

	if (!StartUpGameplayEffects.IsEmpty())
	{
		for (const TSubclassOf<UGameplayEffect>& EffectsClass : StartUpGameplayEffects)
		{
			if (!EffectsClass)
			{
				continue;
			}
			const UGameplayEffect* EffectClassDefaultObject = EffectsClass->GetDefaultObject<UGameplayEffect>();
			InASCToGive->ApplyGameplayEffectToSelf(EffectClassDefaultObject, ApplyLevel, InASCToGive->MakeEffectContext());
		}
	}
}

void UACDataAsset_StartupDataBase::GrantAbilities(const TArray<TSubclassOf<UACGameplayAbility>>& InAbilitiesToGive, UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	if (InAbilitiesToGive.IsEmpty())
	{
		return;
	}

	for (const TSubclassOf<UACGameplayAbility>& Ability : InAbilitiesToGive)
	{
		if (!Ability)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(Ability);
		AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;

		InASCToGive->GiveAbility(AbilitySpec);
	}
}