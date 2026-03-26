// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/Startup/ACDataAsset_EnemyStartupData.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"

void UACDataAsset_EnemyStartupData::GiveToAbilitySystemComponent(UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	Super::GiveToAbilitySystemComponent(InASCToGive, ApplyLevel);

	if (!EnemyGameplayAbility.IsEmpty())
	{
		for (const TSubclassOf<UACGameplayAbility>& AbilityClass : EnemyGameplayAbility)
		{
			if (!AbilityClass)
			{
				continue;
			}
			FGameplayAbilitySpec AbilitySpec(AbilityClass);
			AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();
			AbilitySpec.Level = ApplyLevel;

			InASCToGive->GiveAbility(AbilitySpec);
		}
	}
}