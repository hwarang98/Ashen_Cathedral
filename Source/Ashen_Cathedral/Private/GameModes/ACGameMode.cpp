// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACGameMode.h"
#include "GameModes/ACGameState.h"
#include "Character/ACCharacterBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "DataAssets/Run/ACDataAsset_RunDefinition.h"
#include "DataAssets/Run/ACDataAsset_StageDefinition.h"
#include "Subsystems/ACRunStateSubsystem.h"
#include "Subsystems/ACWeaponSelectionSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#if AC_WEB_DEBUG
	#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
	#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
	#include "Debug/ACRunLogSubsystem.h"
	#include "Debug/ACWebDebugSubsystem.h"
	#include "GameplayAbilitySystem/ACAttributeSet.h"
#endif

#if AC_WEB_DEBUG
namespace ACGameModeInternal
{
	// 런 로그에 남길 현재 선택 무기 태그. 무기 선택이 없으면 빈 태그
	static FGameplayTag GetSelectedWeaponTag(const UGameInstance* GameInstance)
	{
		const UACWeaponSelectionSubsystem* WeaponSelection = GameInstance ? GameInstance->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr;
		const UACDataAsset_WeaponData* WeaponData = WeaponSelection ? WeaponSelection->GetSelectedWeaponData() : nullptr;
		return WeaponData ? WeaponData->WeaponTypeTag : FGameplayTag();
	}
}
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

	// 배치 보스의 BeginPlay가 현재 스테이지를 조회하므로 런 상태는 그보다 먼저 열려 있어야 한다
	BootstrapDebugRunIfNeeded();
}

void AACGameMode::BootstrapDebugRunIfNeeded()
{
	UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (!RunState || RunState->IsRunActive() || !DebugRunDefinition)
	{
		return;
	}

	// 로비를 거치지 않고 아레나에서 바로 PIE를 시작한 경우다. InitGame의 MapName 인자는 직접 PIE 경로에서
	// UEDPIE_ 프리픽스가 섞여 들어오므로 쓰지 않고, 이미 스폰된 월드로 레벨을 판정한다.
	if (!RunState->BeginDebugRunAtLevel(DebugRunDefinition, GetWorld()))
	{
		return;
	}

#if AC_WEB_DEBUG
	// RequestStartRun을 거치지 않는 경로이므로 런 로그도 여기서 열어 준다 — 열리지 않은 런을 닫으려 하면 기록이 어긋난다
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		RunLog->BeginRun(/*InSeed*/ 0, ACGameModeInternal::GetSelectedWeaponTag(GetGameInstance()));
	}
#endif
}

void AACGameMode::HandleMatchHasStarted()
{
	// Super가 NotifyBeginPlay를 호출하므로, 이 아래는 배치 보스의 BeginPlay가 끝난 상태다
	Super::HandleMatchHasStarted();

	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	const UACDataAsset_StageDefinition* Stage = RunState ? RunState->GetCurrentStage() : nullptr;
	if (!Stage)
	{
		return;
	}

	const AACGameState* ACGameState = GetGameState<AACGameState>();
	if (!ACGameState || !ACGameState->GetBossCharacter())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] 스테이지 '%s'에 등록된 보스가 없습니다. 레벨에 보스가 배치되어 있는지, 등록이 거부되지 않았는지 확인하세요."), *Stage->StageID.ToString());
	}
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

	// 한 아레나에는 보스가 하나여야 한다. 조용히 무시하면 두 번째 보스가 사망 델리게이트 없이 살아 있게 된다
	if (ACGameState->GetBossCharacter())
	{
		UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 이미 보스가 등록되어 있는데 '%s'가 추가로 등록을 시도했습니다. 레벨에 보스가 2체 이상 배치되어 있는지 확인하세요."), *InBossCharacter->GetName());
		return;
	}

	// 스테이지 구성이 있으면 배선이 맞는지 확인하고, 어긋나면 등록 자체를 거부해 전투·보상·진행을 모두 막는다
	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (const UACDataAsset_StageDefinition* Stage = RunState ? RunState->GetCurrentStage() : nullptr)
	{
		if (!UACRunStateSubsystem::IsSameLevel(GetWorld(), Stage->LevelAsset))
		{
			UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 현재 맵이 스테이지 '%s'의 LevelAsset과 다릅니다. 보스 등록을 거부했습니다."), *Stage->StageID.ToString());
			return;
		}

		const AACEnemyCharacter* EnemyCharacter = Cast<AACEnemyCharacter>(InBossCharacter);
		const FGameplayTag BossIdentityTag = EnemyCharacter ? EnemyCharacter->GetBossIdentityTag() : FGameplayTag();
		if (!RunState->IsBossValidForCurrentStage(BossIdentityTag))
		{
			UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 스테이지 '%s'는 '%s'를 기대하지만 배치된 보스는 '%s'입니다. 보스 등록을 거부했습니다."),
				*Stage->StageID.ToString(), *Stage->ExpectedBossID.ToString(), *BossIdentityTag.ToString());
			return;
		}
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

		const FGameplayTag WeaponTag = ACGameModeInternal::GetSelectedWeaponTag(GetGameInstance());

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

	if (!InBossCharacter->OnDeathDelegate.IsAlreadyBound(this, &ThisClass::HandleBossDeath))
	{
		InBossCharacter->OnDeathDelegate.AddDynamic(this, &ThisClass::HandleBossDeath);
	}

	if (!InBossCharacter->OnDeathAnimationCompletedDelegate.IsAlreadyBound(this, &ThisClass::HandleBossBattleCompleted))
	{
		InBossCharacter->OnDeathAnimationCompletedDelegate.AddDynamic(this, &ThisClass::HandleBossBattleCompleted);
	}
}

TSoftObjectPtr<UWorld> AACGameMode::ResolveLobbyLevel() const
{
	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (const UACDataAsset_RunDefinition* RunDefinition = RunState ? RunState->GetActiveRunDefinition() : nullptr)
	{
		if (!RunDefinition->LobbyLevel.IsNull())
		{
			return RunDefinition->LobbyLevel;
		}
	}

	return FallbackLobbyLevel;
}

bool AACGameMode::TravelToLevel(const TSoftObjectPtr<UWorld>& TargetLevel)
{
	// OpenLevelBySoftObjectPtr은 빈 포인터를 받아도 경고만 남기고 존재하지 않는 "None" 맵으로 여행한다
	if (TargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 이동할 레벨이 지정되지 않아 레벨 전환을 취소했습니다."));
		return false;
	}

	// 브로드캐스트보다 먼저 잠가야 로딩 화면 대기 중에 다른 이동 요청(사망·출구 재상호작용)이 끼어들지 못한다
	bTravelRequested = true;
	PendingTravelLevel = TargetLevel;

	// 로딩 화면이 페이드 인할 기회를 먼저 준다
	OnStageTravelStartedDelegate.Broadcast(TargetLevel);

	if (TravelScreenDelay <= 0.f)
	{
		ExecutePendingTravel();
		return true;
	}

	GetWorldTimerManager().SetTimer(TravelTimerHandle, this, &AACGameMode::ExecutePendingTravel, TravelScreenDelay, false);
	return true;
}

void AACGameMode::ExecutePendingTravel()
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingTravelLevel);
}

void AACGameMode::RequestProgressAfterBossClear()
{
	if (bTravelRequested)
	{
		return;
	}

	UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (!RunState)
	{
		return;
	}

	if (!RunState->IsCurrentStageCleared())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] 현재 스테이지가 클리어되지 않아 다음 스테이지 진행을 거부했습니다."));
		return;
	}

	if (RunState->GetEffectiveExitPolicy() == EACStageExitPolicy::ForceReturnToLobby)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] 현재 스테이지의 출구 정책이 로비 복귀 전용이라 다음 스테이지 진행을 거부했습니다."));
		return;
	}

	// 인덱스를 전진시키기 전에 목적지가 실제로 열 수 있는 레벨인지 확인한다
	const UACDataAsset_StageDefinition* NextStage = RunState->GetNextStage();
	if (!NextStage)
	{
		// 시퀀스를 끝까지 돌았으므로 Run이 클리어로 끝난다 — 적립분을 확정 지급한다
		SettleRunAndOpenLobby();
		return;
	}

	if (NextStage->LevelAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 다음 스테이지 '%s'에 LevelAsset이 없어 진행하지 않았습니다."), *NextStage->StageID.ToString());
		return;
	}

	const TSoftObjectPtr<UWorld> TargetLevel = NextStage->LevelAsset;
	if (!RunState->AdvanceToNextStage())
	{
		return;
	}

	TravelToLevel(TargetLevel);
}

void AACGameMode::RequestReturnToLobby()
{
	if (bTravelRequested)
	{
		return;
	}

	if (AACGameState* ACGameState = GetGameState<AACGameState>())
	{
		ACGameState->SetBossCharacter(nullptr);
	}

#if AC_WEB_DEBUG
	// 최종 보스를 잡지 않고 스스로 빠져나온 런 — 사망도 클리어도 아니므로 별도 결과로 남긴다
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		RunLog->EndRun(TEXT("extracted"));
	}
#endif

	SettleRunAndOpenLobby();
}

void AACGameMode::SettleRunAndOpenLobby()
{
	// 정산이 ActiveRunDefinition을 지우므로 목적지를 먼저 확보한다
	const TSoftObjectPtr<UWorld> LobbyLevel = ResolveLobbyLevel();
	if (LobbyLevel.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 로비 레벨을 결정할 수 없어 정산과 이동을 모두 취소했습니다. RunDefinition의 LobbyLevel 또는 GameMode의 FallbackLobbyLevel을 설정하세요."));
		return;
	}

	if (UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this))
	{
		RunState->SettleAndEndRun();
	}

	TravelToLevel(LobbyLevel);
}

void AACGameMode::RequestStartRun(UACDataAsset_RunDefinition* RunDefinition)
{
	if (bTravelRequested)
	{
		return;
	}

	// 거부된 시도는 재시도 가능해야 하므로 트래블 래치는 실제 이동 시점에만 소모된다
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

	UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (!RunState)
	{
		return;
	}

	// 이전 Run의 카드·적립분이 남아있지 않도록, 아레나로 넘어가기 전에 Run을 새로 연다.
	// 검증에 실패하면 기존 런 상태를 건드리지 않고 false를 반환한다
	if (!RunState->BeginRunWithDefinition(RunDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACGameMode] RunDefinition 검증에 실패해 전투 시작을 거부했습니다."));
		return;
	}

	const UACDataAsset_StageDefinition* FirstStage = RunState->GetCurrentStage();
	if (!FirstStage || FirstStage->LevelAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[AACGameMode] 첫 스테이지의 레벨을 결정할 수 없어 전투 시작을 취소했습니다."));
		return;
	}

#if AC_WEB_DEBUG
	// 로비에서 아레나로 넘어가는 순간이 런의 시작이다
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		RunLog->BeginRun(/*InSeed*/ 0, ACGameModeInternal::GetSelectedWeaponTag(GetGameInstance()));
	}
#endif

	TravelToLevel(FirstStage->LevelAsset);
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
	if (bTravelRequested)
	{
		return;
	}

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

	// AbandonRun이 ActiveRunDefinition을 지우므로 목적지를 먼저 확보한다
	const TSoftObjectPtr<UWorld> LobbyLevel = ResolveLobbyLevel();

	// 정산 없이 Run을 버린다 — 이번 Run의 적립분과 보상 카드가 모두 소멸한다
	if (UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this))
	{
		RunState->AbandonRun();
	}

	TravelToLevel(LobbyLevel);
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

	UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);

	// 사망 연출 완료가 중복으로 들어와도 보상 적립과 출구 개방이 한 번만 실행되도록 여기서 걸러낸다
	if (!RunState || !RunState->MarkCurrentStageCleared())
	{
		return;
	}

#if AC_WEB_DEBUG
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(this))
	{
		RunLog->EndBossFight(/*bWon*/ true, 0.f);
		// 마지막 스테이지였다면 런 자체가 클리어로 끝난다
		if (RunState->IsCurrentStageFinal())
		{
			RunLog->EndRun(TEXT("cleared"));
		}
	}
#endif

	if (AACEnemyCharacter* DeadBoss = Cast<AACEnemyCharacter>(DeadCharacter))
	{
		const UACDataAsset_StageDefinition* Stage = RunState->GetCurrentStage();
		const float RewardMultiplier = Stage ? Stage->RewardMultiplier : 1.f;

		// 즉시 지급하지 않고 런 지갑에 적립한다 — 로비로 살아 돌아가야 확정된다
		RunState->DepositBossReward(DeadBoss->GetBossRewardData(), DeadBoss, RewardMultiplier);
	}

	OnBossBattleCompletedDelegate.Broadcast(RunState->IsCurrentStageFinal());
}

void AACGameMode::DebugRestartCurrentStage()
{
	if (bTravelRequested)
	{
		return;
	}

	UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);

	// 재시작 후에도 같은 스테이지로 돌아와야 하므로 목적지를 먼저 확보한다
	const UACDataAsset_StageDefinition* Stage = RunState ? RunState->GetCurrentStage() : nullptr;
	TSoftObjectPtr<UWorld> TargetLevel;
	if (Stage)
	{
		TargetLevel = Stage->LevelAsset;
	}

	if (RunState)
	{
		// 스테이지 인덱스와 디버그 런 여부를 유지한 채 이번 런에서 쌓은 카드·적립분만 버린다
		if (!RunState->RestartCurrentStage())
		{
			RunState->AbandonRun();
		}
	}

	if (!TargetLevel.IsNull())
	{
		TravelToLevel(TargetLevel);
		return;
	}

	// 런 구성이 없는 경우에만 현재 맵 이름으로 되돌아간다 (콘솔 전용 폴백 — 로딩 화면 없이 즉시 이동)
	bTravelRequested = true;
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}
