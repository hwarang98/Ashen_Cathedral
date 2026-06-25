// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ACGameMode.generated.h"

class AACCharacterBase;
class AACEnemyCharacter;
class AACPlayerCharacter;

// 보스 사망 연출이 끝나 전투가 완전히 종료됐을 때 브로드캐스트되는 델리게이트. bIsFinalBoss로 마지막 보스인지 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossBattleCompleted, bool, bIsFinalBoss);

/**
 * 보스 전투 오케스트레이션을 담당.
 * StartPlay()에서 레벨에 보스가 배치되어 있지 않으면 NextBossSequence의 첫 보스를
 * AACBossSpawnPoint 위치에 직접 스폰한다(레벨에 보스를 직접 배치하는 기존 방식도 계속 지원).
 * 보스 액터가 BeginPlay 시 RegisterBossCharacter()를 호출하면
 * GameState에 보스를 등록하고 사망 델리게이트를 구독한다.
 * 보스가 죽고 다음 진행을 요청하면(RequestProgressAfterBossClear), NextBossSequence 설정에 따라
 * 같은 맵에서 다음 보스를 스폰하거나, 시퀀스가 끝났으면 Lobby 레벨로 이동한다.
 * 로비 맵에서는 전투 시작 상호작용 액터가 RequestStartRun()을 호출하면 BossArenaLevelName으로 이동하고,
 * 플레이어 캐릭터는 BeginPlay 시 RegisterPlayerCharacter()를 호출해 사망 연출 종료 후 Lobby로 복귀하도록 등록한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AACGameMode();

	virtual void StartPlay() override;

	/**
	 * @brief 보스 액터를 GameState에 등록하고 사망 이벤트를 구독한다.
	 * Level에 배치된 보스 또는 RequestProgressAfterBossClear()로 스폰된 보스가 BeginPlay에서 호출하며,
	 * 이미 보스가 등록된 경우 중복 등록을 막는다.
	 * @param InBossCharacter 등록할 보스 캐릭터
	 */
	void RegisterBossCharacter(AACCharacterBase* InBossCharacter);

	/**
	 * @brief 이번에 등록될(또는 등록된) 보스가 시퀀스상 마지막 보스인지 여부.
	 * NextBossSequence에 남은 보스가 없으면 true. 보상 카드 등록 여부를 결정하는 데 사용된다.
	 */
	bool IsFinalBossPending() const;

	/**
	 * @brief Boss Clear UI의 진행 버튼(Next Boss / Return To Lobby) 클릭 시 UI가 호출한다.
	 * 중복 요청을 막고, 이전 보스 참조를 정리한 뒤, 시퀀스에 따라 다음 보스를 스폰하거나 Lobby로 이동한다.
	 */
	void RequestProgressAfterBossClear();

	/**
	 * @brief 로비의 전투 시작 상호작용 액터가 호출한다.
	 * 중복 요청을 막고 BossArenaLevelName으로 레벨을 전환한다.
	 */
	void RequestStartRun();

	/**
	 * @brief 플레이어 캐릭터를 사망 이벤트 구독 대상으로 등록한다.
	 * 플레이어가 BeginPlay에서 호출하며, 사망 연출이 끝나면 Lobby로 복귀시킨다.
	 * @param InPlayerCharacter 등록할 플레이어 캐릭터
	 */
	void RegisterPlayerCharacter(AACPlayerCharacter* InPlayerCharacter);

	/**
	 * 보스 사망 연출이 끝나 전투가 Completed 상태로 전환됐을 때 브로드캐스트됨.
	 * Boss Clear UI 표시 등 연출 완료 이후에만 동작해야 하는 시스템에서 구독할 것.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FOnBossBattleCompleted OnBossBattleCompletedDelegate;

	FORCEINLINE const FText& GetNextBossButtonText() const { return NextBossButtonText; }
	FORCEINLINE const FText& GetReturnToLobbyButtonText() const { return ReturnToLobbyButtonText; }

protected:
	// 이번 Run에서 순서대로 스폰할 보스 클래스 목록(첫 보스 포함). 비어있으면 레벨에 배치된 보스가 곧 최종 보스다.
	UPROPERTY(EditDefaultsOnly, Category = "Battle|Sequence")
	TArray<TSubclassOf<AACEnemyCharacter>> NextBossSequence;

	// Return To Lobby 시 이동할 레벨 이름
	UPROPERTY(EditDefaultsOnly, Category = "Battle|Sequence")
	FName LobbyLevelName = TEXT("L_Loby");

	// 로비의 전투 시작 상호작용 시 이동할 보스 아레나 레벨 이름
	UPROPERTY(EditDefaultsOnly, Category = "Battle|Sequence")
	FName BossArenaLevelName;

	// Boss Clear UI 진행 버튼에 표시할 문구 (일반 보스 / 최종 보스)
	UPROPERTY(EditDefaultsOnly, Category = "Battle|UI")
	FText NextBossButtonText = FText::FromString(TEXT("Next Boss"));

	UPROPERTY(EditDefaultsOnly, Category = "Battle|UI")
	FText ReturnToLobbyButtonText = FText::FromString(TEXT("Return To Lobby"));

private:
	UFUNCTION()
	void HandleBossDeath(AACCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandleBossBattleCompleted(AACCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandlePlayerDeathCompleted(AACCharacterBase* DeadCharacter);

	// 레벨에 보스가 배치되어 있지 않을 때, AACBossSpawnPoint 위치에 NextBossSequence의 첫 보스를 스폰한다.
	void SpawnInitialBossIfNeeded();

	// 다음 보스를 스폰할 위치 — 가장 최근 등록된 보스의 위치를 재사용
	FTransform CachedBossSpawnTransform;

	// 진행 요청이 이미 처리됐는지 여부 (중복 클릭 방지)
	bool bProgressRequested = false;

	// 전투 시작 요청이 이미 처리됐는지 여부 (중복 클릭 방지)
	bool bRunStartRequested = false;

	// 다음에 스폰할 보스의 NextBossSequence 인덱스
	int32 NextBossSequenceIndex = 0;

public:
	// 디버그용 — 현재 스테이지(맵)를 처음부터 재시작. Boss Clear UI에는 연결되지 않으며 콘솔에서만 호출(Shipping 빌드에서는 동작하지 않음)
	UFUNCTION(Exec)
	void DebugRestartCurrentStage();
};
