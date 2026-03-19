// Fill out your copyright notice in the Description page of Project Settings.


#include "Structs/ACStructTypes.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"

bool FCAInputActionConfig::IsValid() const
{
	return InputTag.IsValid() && InputAction;
}

bool FACPlayerAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}