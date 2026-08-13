// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ACGameMode.generated.h"

class AACCharacterBase;
class AACPlayerCharacter;
class UACDataAsset_RunDefinition;
class UACDataAsset_WeaponData;

// 보스 사망 연출이 끝나 전투가 완전히 종료됐을 때 브로드캐스트되는 델리게이트. bIsFinalBoss로 이번 Run의 마지막 스테이지인지 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossBattleCompleted, bool, bIsFinalBoss);

// 레벨 이동이 확정되어 실제 OpenLevel이 실행되기 직전에 브로드캐스트되는 델리게이트. 로딩 화면을 띄우는 훅으로 사용한다
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageTravelStarted, TSoftObjectPtr<UWorld>, TargetLevel);

/**
 * 현재 로드된 레벨의 보스 전투만 조정한다.
 * Run 전체의 스테이지 구성은 UACDataAsset_RunDefinition이, 진행 상태는 UACRunStateSubsystem이 소유한다 —
 * GameMode는 OpenLevel마다 파괴되므로 런 스코프 데이터를 직접 들고 있지 않는다.
 * 보스는 스테이지가 지정한 아레나 레벨에 미리 배치하는 것이 유일한 방식이며, 런타임 스폰 경로는 없다.
 * 배치된 보스가 BeginPlay에서 RegisterBossCharacter()를 호출하면 현재 맵과 보스 식별 태그가
 * 스테이지 구성과 일치하는지 검증한 뒤에만 GameState에 등록하고 사망 델리게이트를 구독한다.
 * 보스를 잡고 다음 진행을 선택하면(RequestProgressAfterBossClear) 다음 스테이지의 레벨을 OpenLevel 한다.
 * 로비에서는 전투 시작 상호작용 액터가 RunDefinition을 들고 RequestStartRun()을 호출한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AACGameMode();

	/**
	 * @brief 플레이어가 스폰되기 전에 무기 선택 상태와 디버그 런을 초기화한다.
	 *
	 * 선택 무기 스폰 어빌리티는 PossessedBy에서 실행되는데, 이는 StartPlay보다 먼저다.
	 * 따라서 기본 무기 보충은 반드시 이 시점에 끝나 있어야 스폰이 건너뛰어지지 않는다.
	 * 배치 보스의 BeginPlay가 스테이지 정보를 읽으므로 디버그 런도 여기서 열어야 한다.
	 */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 * @brief 모든 액터의 BeginPlay가 끝난 뒤 스테이지 배선을 점검한다.
	 *
	 * @note StartPlay가 아니라 여기인 이유는, AGameMode::StartPlay가 ReadyToStartMatch() 조건부로만
	 *       HandleMatchHasStarted에 도달하기 때문이다. 이 함수의 Super가 NotifyBeginPlay를 호출하므로
	 *       이후 코드는 배치 보스 등록이 끝난 상태를 보장받는다.
	 */
	virtual void HandleMatchHasStarted() override;

	/**
	 * @brief 배치된 보스를 검증하고 GameState에 등록한 뒤 사망 이벤트를 구독한다.
	 *
	 * @param InBossCharacter 등록할 보스 캐릭터
	 * @note 현재 맵이 스테이지의 LevelAsset과 다르거나, 보스의 BossIdentityTag가 ExpectedBossID와 다르거나,
	 *       이미 다른 보스가 등록되어 있으면 오류를 남기고 등록을 거부한다. 등록되지 않은 보스는 사망 델리게이트가
	 *       연결되지 않아 클리어 이벤트도, 보상도, 스테이지 진행도 발생하지 않는다 — 잘못 배선된 스테이지가
	 *       조용히 진행되는 것보다 눈에 띄게 멈추는 편이 안전하다.
	 */
	void RegisterBossCharacter(AACCharacterBase* InBossCharacter);

	/**
	 * @brief 아레나의 스테이지 출구(AACStageExitPoint)가 다음 스테이지 진행을 요청할 때 호출된다.
	 * 현재 스테이지가 클리어됐고 다음 스테이지와 그 레벨이 유효할 때만 인덱스를 전진시키고 레벨을 연다.
	 * 다음 스테이지가 없으면 Run을 완주한 것으로 보고 정산 후 로비로 돌아간다.
	 */
	void RequestProgressAfterBossClear();

	/**
	 * @brief 로비 복귀용 스테이지 출구에 상호작용했을 때 호출된다.
	 * 런 지갑을 정산해 메타 성장에 확정 반영한 뒤 Lobby로 이동한다.
	 * 정산과 함께 Run이 종료되므로 이번 Run의 보상 카드는 모두 소실된다.
	 */
	void RequestReturnToLobby();

	/**
	 * @brief 로비의 전투 시작 상호작용 액터가 호출한다.
	 *
	 * @param RunDefinition 진행할 Run 구성
	 * @note 무기 선택 검증과 RunDefinition 검증을 모두 통과했을 때만 첫 스테이지의 레벨을 연다.
	 *       검증 실패 시 레벨 이동을 실행하지 않으므로 재시도할 수 있다.
	 */
	void RequestStartRun(UACDataAsset_RunDefinition* RunDefinition);

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

	/**
	 * 레벨 이동이 확정된 직후, TravelScreenDelay만큼 기다렸다가 실제 이동이 실행되기 전에 브로드캐스트됨.
	 * 로딩 화면 위젯을 띄우고 페이드 인을 재생하는 지점으로 사용할 것. 모든 이동 경로(스테이지 진행·로비 복귀·사망)가 공유한다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FOnStageTravelStarted OnStageTravelStartedDelegate;

protected:
	// RunDefinition에서 로비 레벨을 얻을 수 없을 때 돌아갈 레벨. 아레나/로비 GameMode 양쪽 모두에 설정할 것
	UPROPERTY(EditDefaultsOnly, Category = "Battle")
	TSoftObjectPtr<UWorld> FallbackLobbyLevel;

	// 이동 확정 후 실제 OpenLevel까지 기다릴 시간(초) — 로딩 화면 페이드 인이 보일 여유. 0이면 같은 프레임에 이동한다
	UPROPERTY(EditDefaultsOnly, Category = "Battle", meta = (ClampMin = "0.0"))
	float TravelScreenDelay = 1.f;

	// 선택된 무기가 없을 때 이 데이터로 초기 선택을 채운다. 비워두면 무기 선택이 필수가 된다
	UPROPERTY(EditDefaultsOnly, Category = "WeaponSelection")
	TObjectPtr<UACDataAsset_WeaponData> DefaultWeaponData;

	// 로비를 거치지 않고 아레나 레벨에서 바로 PIE를 시작했을 때만 사용하는 디버그 런 구성.
	// 현재 레벨에 해당하는 스테이지를 찾아 그 지점부터 런을 연다. 로비 GameMode에서는 비워 둘 것
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TObjectPtr<UACDataAsset_RunDefinition> DebugRunDefinition;

private:
	// 레벨 시작 시 무기 선택 상태를 정리한다 (교체 진행 플래그 해제, 기본 무기 보충)
	void InitializeWeaponSelectionForLevel();

	// 활성 런이 없고 DebugRunDefinition이 설정되어 있으면 현재 레벨에 해당하는 스테이지부터 디버그 런을 연다
	void BootstrapDebugRunIfNeeded();

	UFUNCTION()
	void HandleBossDeath(AACCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandleBossBattleCompleted(AACCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandlePlayerDeathCompleted(AACCharacterBase* DeadCharacter);

	// 런 지갑을 정산해 메타 성장에 확정 반영하고 Lobby로 이동한다. 로비 복귀 경로 공통 처리
	void SettleRunAndOpenLobby();

	/**
	 * @brief 돌아갈 로비 레벨을 결정한다.
	 *
	 * @return RunDefinition의 LobbyLevel, 없으면 FallbackLobbyLevel. 둘 다 없으면 null
	 * @note 정산·포기 처리가 ActiveRunDefinition을 지우므로 반드시 그보다 먼저 호출해 값을 확보해야 한다.
	 */
	TSoftObjectPtr<UWorld> ResolveLobbyLevel() const;

	/**
	 * @brief 지정한 레벨로의 이동을 예약한다.
	 *
	 * @param TargetLevel 이동할 레벨
	 * @return 이동이 예약됐으면 true
	 * @note OpenLevelBySoftObjectPtr에는 null 가드가 없어 빈 포인터를 넘기면 존재하지 않는 "None" 맵으로
	 *       여행해 버리므로 여기서 막는다. 예약과 동시에 OnStageTravelStartedDelegate를 브로드캐스트해
	 *       로딩 화면이 페이드 인할 기회를 주고, TravelScreenDelay 뒤에 실제 이동을 실행한다.
	 *       bTravelRequested가 브로드캐스트보다 먼저 잠기므로 대기 중의 중복 이동 요청은 기존과 동일하게 막힌다.
	 */
	bool TravelToLevel(const TSoftObjectPtr<UWorld>& TargetLevel);

	// TravelScreenDelay 타이머가 만료되면 예약된 레벨로 실제 이동한다
	void ExecutePendingTravel();

	// 예약된 이동의 목적지. TravelToLevel에서 기록되고 ExecutePendingTravel에서 소비된다
	TSoftObjectPtr<UWorld> PendingTravelLevel;

	// 지연 이동 타이머
	FTimerHandle TravelTimerHandle;

	// 어떤 경로로든 레벨 이동이 한 번 요청되면 잠긴다. 같은 프레임에 두 요청이 겹치면 마지막 것만 남으므로
	// (SetClientTravel은 다음 틱에 처리된다) 모든 이동 경로가 이 래치를 공유한다. GameMode는 레벨과 함께 파괴되므로 해제하지 않는다
	bool bTravelRequested = false;

public:
	// 디버그용 — 현재 스테이지를 처음부터 재시작. Boss Clear UI에는 연결되지 않으며 콘솔에서만 호출(Shipping 빌드에서는 동작하지 않음)
	UFUNCTION(Exec)
	void DebugRestartCurrentStage();
};
