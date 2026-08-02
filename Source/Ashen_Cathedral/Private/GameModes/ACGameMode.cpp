// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACGameMode.h"
#include "GameModes/ACBossSpawnPoint.h"
#include "GameModes/ACGameState.h"
#include "Character/ACCharacterBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "Subsystems/ACWeaponSelectionSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#if AC_WEB_DEBUG
	#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
	#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
	#include "Debug/ACRunLogSubsystem.h"
	#include "Debug/ACWebDebugSubsystem.h"
	#include "GameplayAbilitySystem/ACAttributeSet.h"
#endif

AACGameMode::AACGameMode()
{
	GameStateClass = AACGameState::StaticClass();
}

void AACGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// PossessedBy(플레이어 스폰)보다 먼저 실행되므로 선택 무기가 여기서 확정되어야 한다
	InitializeWeaponSelectionForLevel();
}

void AACGameMode::StartPlay()
{
	Super::StartPlay();

	SpawnInitialBossIfNeeded();
}

void AACGameMode::InitializeWeaponSelectionForLevel()
{
	UACWeaponSelectionSubsystem* WeaponSelectionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr;
	if (!WeaponSelectionSubsystem)
	{
		return;
	}

	// 레벨 전환 도중 교체가 중단됐다면 플래그가 남아 아무것도 못 고르게 되므로 정리한다
	WeaponSelectionSubsystem->SetWeaponChangeInProgress(false);

	if (!WeaponSelectionSubsystem->HasSelectedWeaponData() && DefaultWeaponData)
	{
		WeaponSelectionSubsystem->SetSelectedWeaponData(DefaultWeaponData);
	}
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

#if AC_WEB_DEBUG
	// 웹 디버그 — 보스전 단위로 타임라인의 시간 원점과 런 통계 슬롯을 연다
	{
		FGameplayTag BossId;
		if (const AACEnemyCharacter* Boss = Cast<AACEnemyCharacter>(InBossCharacter))
		{
			if (const UACDataAsset_BossReward* RewardData = Boss->GetBossRewardData())
			{
				BossId = RewardData->BossID;
			}
		}

		FGameplayTag WeaponTag;
		if (const UACWeaponSelectionSubsystem* WeaponSelection = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr)
		{
			if (const UACDataAsset_WeaponData* WeaponData = WeaponSelection->GetSelectedWeaponData())
			{
				WeaponTag = WeaponData->WeaponTypeTag;
			}
		}

		int32 Attempt = 0;
		int32 RunSeed = 0;
		if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
		{
			Attempt = RunLog->BeginBossFight(BossId);
			RunSeed = RunLog->GetSeed();
		}
		if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(this))
		{
			WebDebug->BeginCombat(BossId, Attempt, WeaponTag, RunSeed);
		}
	}
#endif

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

	// 보스를 스폰하기 전에 플레이어를 먼저 옮겨, 새 보스가 등장할 때 아레나 초기 배치가 재현되도록 한다.
	TeleportPlayerToPlayerStart();

	GetWorld()->SpawnActor<AACEnemyCharacter>(NextBossClass, CachedBossSpawnTransform);
}

void AACGameMode::TeleportPlayerToPlayerStart()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	APawn* PlayerPawn = PlayerController->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	AActor* PlayerStart = FindPlayerStart(PlayerController);
	if (!PlayerStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] PlayerStart가 없어 플레이어를 시작 위치로 되돌리지 못했습니다."));
		return;
	}

	// 이동 직후 남은 속도로 미끄러지지 않도록 정지시킨다.
	if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn))
	{
		if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	const FRotator StartRotation = PlayerStart->GetActorRotation();

	// TeleportTo는 도착 지점이 막혀 있으면 인접한 빈 공간을 찾아준다.
	PlayerPawn->TeleportTo(PlayerStart->GetActorLocation(), StartRotation);

	// 카메라(컨트롤 회전)도 함께 맞춰야 플레이어가 보스 쪽을 바라보며 시작한다.
	PlayerController->SetControlRotation(StartRotation);
}

void AACGameMode::RequestStartRun()
{
	if (bRunStartRequested)
	{
		return;
	}

	// 거부된 시도는 재시도 가능해야 하므로 요청 플래그는 모든 검증을 통과한 뒤에 소모한다
	if (UACWeaponSelectionSubsystem* WeaponSelectionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr)
	{
		if (WeaponSelectionSubsystem->IsWeaponChangeInProgress())
		{
			UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] 무기 교체가 진행 중이라 전투 시작을 거부했습니다."));
			return;
		}

		if (!WeaponSelectionSubsystem->HasSelectedWeaponData())
		{
			UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] 선택된 무기가 없어 전투 시작을 거부했습니다. 로비에서 무기를 먼저 선택하세요."));
			return;
		}
	}

	bRunStartRequested = true;

#if AC_WEB_DEBUG
	// 로비에서 아레나로 넘어가는 순간이 런의 시작이다
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		FGameplayTag WeaponTag;
		if (const UACWeaponSelectionSubsystem* WeaponSelection = GetGameInstance() ? GetGameInstance()->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr)
		{
			if (const UACDataAsset_WeaponData* WeaponData = WeaponSelection->GetSelectedWeaponData())
			{
				WeaponTag = WeaponData->WeaponTypeTag;
			}
		}
		RunLog->BeginRun(/*InSeed*/ 0, WeaponTag);
	}
#endif

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
#if AC_WEB_DEBUG
	// 런 종료(사망) — 보스 잔여 체력을 남기고 Saved/RunLogs 에 기록한다
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		float BossHealthPct = 1.f;
		const AACGameState* ACGameState = GetGameState<AACGameState>();
		const AACCharacterBase* Boss = ACGameState ? ACGameState->GetBossCharacter() : nullptr;
		if (const UACAttributeSet* BossAttributes = Boss ? Boss->GetACAttributeSet() : nullptr)
		{
			BossHealthPct = BossAttributes->GetMaxHealth() > 0.f ? BossAttributes->GetHealth() / BossAttributes->GetMaxHealth() : 0.f;
		}
		RunLog->EndBossFight(/*bWon*/ false, BossHealthPct);
		RunLog->EndRun(TEXT("died"));
	}
#endif

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

#if AC_WEB_DEBUG
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		RunLog->EndBossFight(/*bWon*/ true, 0.f);
		// 마지막 보스였다면 런 자체가 클리어로 끝난다
		if (IsFinalBossPending())
		{
			RunLog->EndRun(TEXT("cleared"));
		}
	}
#endif

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
