// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "Controllers/ACPlayerController.h"

AACPlayerCharacter* UACPlayerGameplayAbility::GetPlayerCharacterFromActorInfo() const
{
	if (!CachedPlayerCharacter.IsValid())
	{
		CachedPlayerCharacter = Cast<AACPlayerCharacter>(CurrentActorInfo->AvatarActor.Get());
	}

	return CachedPlayerCharacter.IsValid() ? CachedPlayerCharacter.Get() : nullptr;
}


UPlayerCombatComponent* UACPlayerGameplayAbility::GetPlayerCombatComponentFromActorInfo() const
{
	if (const AACPlayerCharacter* PlayerCharacter = GetPlayerCharacterFromActorInfo())
	{
		return Cast<UPlayerCombatComponent>(PlayerCharacter->GetPawnCombatComponent());
	}
	return nullptr;
}

AACPlayerController* UACPlayerGameplayAbility::GetPlayerControllerFromActorInfo()
{
	if (!CachedPlayerController.IsValid())
	{
		CachedPlayerController = Cast<AACPlayerController>(CurrentActorInfo->PlayerController);
	}

	return CachedPlayerController.IsValid() ? CachedPlayerController.Get() : nullptr;
}