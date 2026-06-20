// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACGameMode.h"
#include "GameModes/ACGameState.h"
#include "Character/ACCharacterBase.h"

AACGameMode::AACGameMode()
{
	GameStateClass = AACGameState::StaticClass();
}

void AACGameMode::RegisterBossCharacter(AACCharacterBase* InBossCharacter)
{
	if (!InBossCharacter)
	{
		return;
	}

	AACGameState* ACGameState = GetGameState<AACGameState>();
	if (!ACGameState)
	{
		return;
	}

	// 이미 보스가 등록된 경우 중복 초기화 방지
	if (ACGameState->GetBossCharacter())
	{
		return;
	}

	ACGameState->SetBossCharacter(InBossCharacter);
	ACGameState->SetBattleState(EACBattleState::BossBattleInProgress);

	if (!InBossCharacter->OnDeathDelegate.IsAlreadyBound(this, &ThisClass::HandleBossDeath))
	{
		InBossCharacter->OnDeathDelegate.AddDynamic(this, &ThisClass::HandleBossDeath);
	}
}

void AACGameMode::HandleBossDeath(AACCharacterBase* DeadCharacter)
{
	AACGameState* ACGameState = GetGameState<AACGameState>();
	if (!ACGameState)
	{
		return;
	}

	ACGameState->SetBattleState(EACBattleState::BossDefeated);
}