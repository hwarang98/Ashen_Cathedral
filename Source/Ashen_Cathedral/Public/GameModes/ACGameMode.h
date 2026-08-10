// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ACGameMode.generated.h"

class AACCharacterBase;
class AACEnemyCharacter;
class AACPlayerCharacter;
class UACDataAsset_WeaponData;

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
 * GameMode는 레벨 전환 때 파괴되므로 보스 진행도·보상 카드·미정산 보상 같은 Run 스코프 상태는
 * 직접 들고 있지 않고 UACRunStateSubsystem에 위임한다.
 * 로비 맵에서는 전투 시작 상호작용 액터가 RequestStartRun()을 호출하면 BossArenaLevelName으로 이동하고,
 * 플레이어 캐릭터는 BeginPlay 시 RegisterPlayerCharacter()를 호출해 사망 연출 종료 후 Lobby로 복귀하도록 등록한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AACGameMode();

	/**
	 * @brief 플레이어가 스폰되기 전에 무기 선택 상태를 초기화한다.
	 *
	 * 선택 무기 스폰 어빌리티는 PossessedBy에서 실행되는데, 이는 StartPlay보다 먼저다.
	 * 따라서 기본 무기 보충은 반드시 이 시점에 끝나 있어야 스폰이 건너뛰어지지 않는다.
	 */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

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
	 * @brief 아레나의 스테이지 출구 오브젝트(AACStageExitPoint)에 상호작용했을 때 호출된다.
	 * 중복 요청을 막고, 이전 보스 참조를 정리한 뒤, 시퀀스에 따라 다음 보스를 스폰하거나 Lobby로 이동한다.
	 * 다음 보스로 이어질 때는 Run이 유지되므로 보상 카드와 런 지갑이 그대로 넘어간다.
	 */
	void RequestProgressAfterBossClear();

	/**
	 * @brief 로비 복귀용 스테이지 출구에 상호작용했을 때 호출된다.
	 * 중복 요청을 막고, 런 지갑을 정산해 메타 성장에 확정 반영한 뒤 Lobby로 이동한다.
	 * 정산과 함께 Run이 종료되므로 이번 Run의 보상 카드는 모두 소실된다.
	 */
	void RequestReturnToLobby();

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

	/**
	 * 이번 Run에서 클리어한 보스 순번(0-based)별 보상 배수.
	 * 깊이 들어갈수록 커지게 두면 앞 스테이지 반복 파밍이 손해가 되어 로비 복귀 선택이 실제 판단이 된다.
	 * 배열보다 깊이 진행하면 마지막 값을 계속 사용하며, 비워두면 배수 없이 원래 금액을 적립한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Battle|Sequence")
	TArray<float> StageRewardMultipliers = { 1.f, 1.5f, 2.5f };

	// 선택된 무기가 없을 때 이 데이터로 초기 선택을 채운다. 비워두면 무기 선택이 필수가 된다
	UPROPERTY(EditDefaultsOnly, Category = "WeaponSelection")
	TObjectPtr<UACDataAsset_WeaponData> DefaultWeaponData;

private:
	// 레벨 시작 시 무기 선택 상태를 정리한다 (교체 진행 플래그 해제, 기본 무기 보충)
	void InitializeWeaponSelectionForLevel();

	UFUNCTION()
	void HandleBossDeath(AACCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandleBossBattleCompleted(AACCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandlePlayerDeathCompleted(AACCharacterBase* DeadCharacter);

	// 레벨에 보스가 배치되어 있지 않을 때, AACBossSpawnPoint 위치에 NextBossSequence의 첫 보스를 스폰한다.
	void SpawnInitialBossIfNeeded();

	// 아레나 레벨에서 바로 PIE를 시작하면 로비의 RequestStartRun()을 거치지 않으므로, 여기서 Run을 대신 열어 준다.
	void EnsureRunStarted();

	// 런 지갑을 정산해 메타 성장에 확정 반영하고 Lobby로 이동한다. 로비 복귀 경로 공통 처리
	void SettleRunAndOpenLobby();

	// 이번에 클리어한 보스의 순번에 해당하는 보상 배수. StageRewardMultipliers가 비어있으면 1.0
	float GetCurrentStageRewardMultiplier() const;

	// 다음 보스를 스폰하기 전에 플레이어를 PlayerStart 위치로 되돌린다(같은 맵에서 스테이지가 이어지므로 수동 복귀가 필요).
	void TeleportPlayerToPlayerStart();

	// 다음 보스를 스폰할 위치 — 가장 최근 등록된 보스의 위치를 재사용
	FTransform CachedBossSpawnTransform;

	// 진행 요청이 이미 처리됐는지 여부 (중복 클릭 방지)
	bool bProgressRequested = false;

	// 전투 시작 요청이 이미 처리됐는지 여부 (중복 클릭 방지)
	bool bRunStartRequested = false;

public:
	// 디버그용 — 현재 스테이지(맵)를 처음부터 재시작. Boss Clear UI에는 연결되지 않으며 콘솔에서만 호출(Shipping 빌드에서는 동작하지 않음)
	UFUNCTION(Exec)
	void DebugRestartCurrentStage();
};
