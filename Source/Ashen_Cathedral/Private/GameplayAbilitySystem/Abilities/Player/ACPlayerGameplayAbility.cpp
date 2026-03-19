// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/Combat/PlayerCombatComponent.h"

AACPlayerCharacter* UACPlayerGameplayAbility::GetPlayerCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACPlayerCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UPlayerCombatComponent* UACPlayerGameplayAbility::GetPlayerCombatComponentFromActorInfo() const
{
	if (const AACPlayerCharacter* PlayerCharacter = GetPlayerCharacterFromActorInfo())
	{
		return Cast<UPlayerCombatComponent>(PlayerCharacter->GetPawnCombatComponent());
	}
	return nullptr;
}