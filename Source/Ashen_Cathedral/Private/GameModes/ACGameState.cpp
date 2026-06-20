// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACGameState.h"

void AACGameState::SetBattleState(EACBattleState NewState)
{
	BattleState = NewState;
}

void AACGameState::SetBossCharacter(AACCharacterBase* InBossCharacter)
{
	BossCharacter = InBossCharacter;
}
