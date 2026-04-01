// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ACBTTask_ActivateAbilityByTag.h"

#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACBTTask_ActivateAbilityByTag::UACBTTask_ActivateAbilityByTag()
{
	NodeName = TEXT("BTTask_ActivateAbilityByTag");
}

void UACBTTask_ActivateAbilityByTag::OnEnemyExecuteTask()
{
	OwningEnemyCharacter->GetACAbilitySystemComponent()->TryActivateAbilitiesByTag(AbilityTagToActivate);
}