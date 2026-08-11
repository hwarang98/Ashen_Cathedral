// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"

#include "Character/Enemy/ACEnemyCharacter.h"

AACEnemyCharacter* UACEnemyGameplayAbility::GetEnemyCharacterFromActorInfo() const
{
	if (!CachedEnemyCharacter.IsValid())
	{
		CachedEnemyCharacter = Cast<AACEnemyCharacter>(CurrentActorInfo->AvatarActor.Get());
	}

	return CachedEnemyCharacter.IsValid() ? CachedEnemyCharacter.Get() : nullptr;
}

UEnemyCombatComponent* UACEnemyGameplayAbility::GetEnemyCombatComponentFromActorInfo() const
{
	if (const AACEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo())
	{
		return Cast<UEnemyCombatComponent>(EnemyCharacter->GetPawnCombatComponent());
	}

	return nullptr;
}
