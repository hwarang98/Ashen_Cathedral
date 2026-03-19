// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/ACEnemyCharacter.h"

AACEnemyCharacter::AACEnemyCharacter()
{
	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("Enemy Combat Component"));
}

UPawnCombatComponent* AACEnemyCharacter::GetPawnCombatComponent() const
{
	return EnemyCombatComponent;
}
