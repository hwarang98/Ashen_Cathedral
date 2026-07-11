// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACGameMode.h"
#include "GameModes/ACBossSpawnPoint.h"
#include "GameModes/ACGameState.h"
#include "Character/ACCharacterBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "Kismet/GameplayStatics.h"

AACGameMode::AACGameMode()
{
	GameStateClass = AACGameState::StaticClass();
}

void AACGameMode::StartPlay()
{
	Super::StartPlay();

	SpawnInitialBossIfNeeded();
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

	// 다음 보스를 스폰할 위치로 재사용 (Scale은 다음 보스 자신의 BP 기본값을 따르도록 제외)
	CachedBossSpawnTransform = FTransform(InBossCharacter->GetActorRotation(), InBossCharacter->GetActorLocation());

	if (!InBossCharacter->OnDeathDelegate.IsAlreadyBound(this, &ThisClass::HandleBossDeath))
	{
		InBossCharacter->OnDeathDelegate.AddDynamic(this, &ThisClass::HandleBossDeath);
	}

	if (!InBossCharacter->OnDeathAnimationCompletedDelegate.IsAlreadyBound(this, &ThisClass::HandleBossBattleCompleted))
	{
		InBossCharacter->OnDeathAnimationCompletedDelegate.AddDynamic(this, &ThisClass::HandleBossBattleCompleted);
	}
}

bool AACGameMode::IsFinalBossPending() const
{
	return NextBossSequenceIndex >= NextBossSequence.Num();
}

void AACGameMode::RequestProgressAfterBossClear()
{
	if (bProgressRequested)
	{
		return;
	}
	bProgressRequested = true;

	AACGameState* ACGameState = GetGameState<AACGameState>();
	if (!ACGameState)
	{
		return;
	}

	// 이전 보스 참조 정리 — 다음 보스가 RegisterBossCharacter의 중복 등록 가드에 막히지 않도록 함
	if (AACCharacterBase* DeadBoss = ACGameState->GetBossCharacter())
	{
		DeadBoss->Destroy();
	}
	ACGameState->SetBossCharacter(nullptr);

	if (IsFinalBossPending())
	{
		UGameplayStatics::OpenLevel(this, LobbyLevelName);
		return;
	}

	TSubclassOf<AACEnemyCharacter> NextBossClass = NextBossSequence[NextBossSequenceIndex++];
	if (!NextBossClass)
	{
		return;
	}

	bProgressRequested = false;
	GetWorld()->SpawnActor<AACEnemyCharacter>(NextBossClass, CachedBossSpawnTransform);
}

void AACGameMode::RequestStartRun()
{
	if (bRunStartRequested)
	{
		return;
	}
	bRunStartRequested = true;

	UGameplayStatics::OpenLevel(this, BossArenaLevelName);
}

void AACGameMode::RegisterPlayerCharacter(AACPlayerCharacter* InPlayerCharacter)
{
	if (!InPlayerCharacter)
	{
		return;
	}

	if (!InPlayerCharacter->OnDeathAnimationCompletedDelegate.IsAlreadyBound(this, &ThisClass::HandlePlayerDeathCompleted))
	{
		InPlayerCharacter->OnDeathAnimationCompletedDelegate.AddDynamic(this, &ThisClass::HandlePlayerDeathCompleted);
	}
}

void AACGameMode::HandlePlayerDeathCompleted(AACCharacterBase* DeadCharacter)
{
	UGameplayStatics::OpenLevel(this, LobbyLevelName);
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

void AACGameMode::HandleBossBattleCompleted(AACCharacterBase* DeadCharacter)
{
	AACGameState* ACGameState = GetGameState<AACGameState>();
	if (!ACGameState)
	{
		return;
	}

	ACGameState->SetBattleState(EACBattleState::Completed);

	if (AACEnemyCharacter* DeadBoss = Cast<AACEnemyCharacter>(DeadCharacter))
	{
		if (UACMetaProgressionSubsystem* MetaProgressionSubsystem = GetGameInstance()->GetSubsystem<UACMetaProgressionSubsystem>())
		{
			MetaProgressionSubsystem->GrantBossReward(DeadBoss->GetBossRewardData(), DeadBoss);
		}
	}

	OnBossBattleCompletedDelegate.Broadcast(IsFinalBossPending());
}

void AACGameMode::SpawnInitialBossIfNeeded()
{
	AACGameState* ACGameState = GetGameState<AACGameState>();
	if (!ACGameState || ACGameState->GetBossCharacter())
	{
		// 레벨에 보스가 이미 배치되어 BeginPlay에서 등록을 마쳤다면 추가로 스폰하지 않는다.
		return;
	}

	if (NextBossSequence.IsEmpty())
	{
		return;
	}

	AACBossSpawnPoint* SpawnPoint = Cast<AACBossSpawnPoint>(UGameplayStatics::GetActorOfClass(GetWorld(), AACBossSpawnPoint::StaticClass()));
	if (!SpawnPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] AACBossSpawnPoint가 레벨에 없어 첫 보스를 스폰할 수 없습니다."));
		return;
	}

	TSubclassOf<AACEnemyCharacter> FirstBossClass = NextBossSequence[NextBossSequenceIndex++];
	if (!FirstBossClass)
	{
		return;
	}

	GetWorld()->SpawnActor<AACEnemyCharacter>(FirstBossClass, SpawnPoint->GetActorTransform());
}

void AACGameMode::DebugRestartCurrentStage()
{
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}
