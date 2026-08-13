// 한 Run 동안만 유지되는 상태(스테이지 진행·보상 카드·미정산 보상)를 레벨 전환 너머로 보관하는 GameInstanceSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Enums/ACEnums.h"
#include "ACRunStateSubsystem.generated.h"

class UACDataAsset_BossReward;
class UACDataAsset_RunDefinition;
class UACDataAsset_StageDefinition;
class UWorld;

// 미정산 보상(런 지갑)이 변경될 때 브로드캐스트된다. 출구 프롬프트 등이 "지금 나가면 받을 금액"을 표시할 때 구독한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPendingRewardChanged, FGameplayTag, CurrencyTag, int32, PendingAmount);

/**
 * @brief 한 Run에 속하는 휘발성 상태를 보관하는 Subsystem.
 *
 * GameInstance 생명주기를 따르므로 아레나 -> 아레나 레벨 전환(OpenLevel)에도 값이 유지된다.
 * 플레이어 캐릭터와 GameMode는 레벨 전환 때 파괴되므로, 레벨을 넘어야 하는 런 상태는 모두 여기에 둔다.
 *
 * 스테이지 구성은 UACDataAsset_RunDefinition이 소유하고 이 Subsystem은 진행 상태만 갖는다.
 * 스테이지 인덱스는 레벨 로드나 보스 등록으로 증가하지 않으며, 클리어 후 다음 스테이지가 유효함이
 * 확인된 뒤 AdvanceToNextStage()가 불릴 때만 증가한다.
 *
 * 보상은 보스를 잡는 즉시 확정되지 않고 런 지갑(PendingCurrencies)에 적립되며,
 * 로비로 무사히 복귀할 때(SettleAndEndRun) 비로소 UACMetaProgressionSubsystem에 확정 지급된다.
 * 사망 시(AbandonRun)에는 적립분이 전부 소멸한다 — 이 비대칭이 "더 깊이 갈 것인가"의 위험 대 보상을 만든다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACRunStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// WorldContextObject가 속한 GameInstance의 인스턴스를 반환한다. 없으면 nullptr
	static UACRunStateSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * @brief 현재 로드된 월드가 지정한 레벨 에셋과 같은 레벨인지 판정한다.
	 *
	 * @param World 비교할 현재 월드
	 * @param Target 스테이지가 지정한 레벨 에셋
	 * @return 같은 레벨이면 true
	 * @note FSoftObjectPath::FixupForPIE는 아직 로드되지 않은 레벨에 동작하지 않으므로, 반대로 현재 월드 쪽의
	 *       PIE 프리픽스를 벗겨 긴 패키지 경로끼리 비교한다. 짧은 맵 이름이 아니라 전체 경로를 비교하므로
	 *       폴더가 다른 동명 맵을 혼동하지 않는다.
	 */
	static bool IsSameLevel(const UWorld* World, const TSoftObjectPtr<UWorld>& Target);

	#pragma region Run Lifecycle
	/**
	 * @brief RunDefinition을 검증하고 새 Run을 시작한다.
	 *
	 * @param RunDefinition 진행할 스테이지 구성
	 * @return 검증을 모두 통과해 런이 열렸으면 true
	 * @note 트랜잭션으로 동작한다 — 모든 스테이지와 레벨 에셋을 먼저 검증하고, 전부 통과했을 때만 기존 상태를
	 *       초기화한다. 실패하면 진행 중이던 런 상태를 전혀 건드리지 않는다. 비어 있는 스테이지가 하나라도
	 *       있으면 걸러내지 않고 전체를 실패 처리한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "RunState")
	bool BeginRunWithDefinition(UACDataAsset_RunDefinition* RunDefinition);

	/**
	 * @brief 디버그 전용 — 현재 로드된 레벨에 해당하는 스테이지부터 Run을 연다.
	 *
	 * @param RunDefinition 진행할 스테이지 구성
	 * @param CurrentWorld 현재 로드된 월드
	 * @return 일치하는 스테이지를 찾아 런이 열렸으면 true
	 * @note 아레나 레벨에서 로비를 거치지 않고 바로 PIE를 시작하는 개발 흐름을 위한 경로다. 일치하는 스테이지를
	 *       찾지 못하면 인덱스 0으로 폴백하지 않고 런을 열지 않는다. 이 경로로 연 런은 SettleAndEndRun에서
	 *       MetaProgression/SaveGame에 아무것도 커밋하지 않는다.
	 */
	bool BeginDebugRunAtLevel(UACDataAsset_RunDefinition* RunDefinition, const UWorld* CurrentWorld);

	/**
	 * @brief 현재 스테이지를 처음부터 다시 시작한다 — 구성·인덱스·디버그 여부는 유지한다.
	 *
	 * @return 활성 런이 있어 재시작이 처리됐으면 true
	 * @note 이번 런의 카드·적립분·클리어 플래그는 초기화되지만 CurrentStageIndex와 bDebugRun은 보존된다.
	 */
	bool RestartCurrentStage();

	/**
	 * @brief Run을 정산하고 종료한다 — 런 지갑의 재화와 첫 클리어 기록을 메타 성장에 확정 반영한다.
	 * 로비 복귀 출구와 최종 스테이지 클리어 경로에서 호출한다.
	 * @note 디버그 런에서는 커밋을 모두 건너뛴다 — SaveGame 영구 데이터를 변경하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "RunState")
	void SettleAndEndRun();

	/**
	 * @brief Run을 정산 없이 종료한다 — 런 지갑의 적립분과 첫 클리어 기록이 모두 소멸한다.
	 * 플레이어 사망 시 호출한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "RunState")
	void AbandonRun();

	UFUNCTION(BlueprintPure, Category = "RunState")
	FORCEINLINE bool IsRunActive() const { return bRunActive; }

	// 디버그 경로로 연 런인지 여부. true면 정산 시 영구 데이터를 커밋하지 않는다
	UFUNCTION(BlueprintPure, Category = "RunState")
	FORCEINLINE bool IsDebugRun() const { return bDebugRun; }

	FORCEINLINE const UACDataAsset_RunDefinition* GetActiveRunDefinition() const { return ActiveRunDefinition; }
	#pragma endregion

	#pragma region Stage Progression
	// 현재 진행 중인 스테이지. 활성 런이 없으면 nullptr
	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	const UACDataAsset_StageDefinition* GetCurrentStage() const;

	// 다음에 이어질 스테이지. 현재가 마지막이면 nullptr
	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	const UACDataAsset_StageDefinition* GetNextStage() const;

	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	bool HasNextStage() const;

	/**
	 * @brief 현재 스테이지가 이 Run의 마지막인지 여부.
	 *
	 * @return 마지막 스테이지면 true
	 * @note 활성 스테이지가 없으면 false를 반환한다. 런 구성 없이 아레나에서 바로 PIE를 시작한 경우에도
	 *       보상 카드가 정상 동작하도록 하기 위함이다 — "구성이 없으니 최종 보스"로 판정하면 카드가 죽는다.
	 */
	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	bool IsCurrentStageFinal() const;

	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	FORCEINLINE bool IsCurrentStageCleared() const { return bCurrentStageCleared; }

	// 이번 Run에서 지금까지 클리어한 스테이지 수 (bCountsAsRunStage가 켜진 스테이지만 집계)
	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	FORCEINLINE int32 GetClearedBossCount() const { return ClearedBossCount; }

	/**
	 * @brief 레벨에 배치된 보스가 현재 스테이지가 기대하는 보스인지 판정한다.
	 *
	 * @param BossIdentityTag 배치된 보스의 AACEnemyCharacter::BossIdentityTag
	 * @return 스테이지 구성과 일치하면 true
	 * @note 활성 스테이지가 없거나 ExpectedBossID가 설정되지 않았으면 검사하지 않고 true를 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	bool IsBossValidForCurrentStage(const FGameplayTag& BossIdentityTag) const;

	/**
	 * @brief 잘못 설정된 조합을 안전한 값으로 보정한, 실제로 적용할 출구 정책.
	 *
	 * @return 보정된 정책. 마지막 스테이지에 AutoNextStage가 설정되어 있으면 ForceReturnToLobby로 폴백한다
	 * @note 활성 스테이지가 없으면 NormalChoice를 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "RunState|Stage")
	EACStageExitPolicy GetEffectiveExitPolicy() const;

	/**
	 * @brief 현재 스테이지를 클리어 상태로 표시한다. 보스 사망 연출이 완료됐을 때만 호출한다.
	 *
	 * @return 이번 호출로 처음 클리어 처리됐으면 true
	 * @note 멱등하다 — 이미 클리어된 스테이지에서는 false를 반환하므로, 호출자는 반환값으로 보상 지급과
	 *       완료 이벤트가 중복 실행되는 것을 막을 수 있다.
	 */
	bool MarkCurrentStageCleared();

	/**
	 * @brief 다음 스테이지로 진행한다.
	 *
	 * @return 인덱스가 실제로 전진했으면 true
	 * @note 현재 스테이지가 클리어됐고, 다음 스테이지가 있으며, 그 LevelAsset이 유효할 때만 성공한다.
	 *       실패하면 CurrentStageIndex를 전혀 건드리지 않는다.
	 */
	bool AdvanceToNextStage();
	#pragma endregion

	#pragma region Reward Cards
	// 이번 Run에서 획득한 카드 ID -> 중첩 수. 레벨 전환 후 GE·Ability를 재적용할 때 원본이 된다.
	FORCEINLINE const TMap<FName, int32>& GetAcquiredCardStacks() const { return AcquiredCardStacks; }

	UFUNCTION(BlueprintPure, Category = "RunState|Card")
	int32 GetCardStack(FName CardID) const;

	/**
	 * @brief 카드 중첩을 1 올린다.
	 * @param CardID 획득한 카드 ID
	 * @param bIsLegendary 전설 카드인지 여부 — true면 이번 Run의 전설 획득 플래그를 세운다
	 */
	void AddCardStack(FName CardID, bool bIsLegendary);

	UFUNCTION(BlueprintPure, Category = "RunState|Card")
	FORCEINLINE bool IsLegendaryUsed() const { return bLegendaryUsed; }
	#pragma endregion

	#pragma region Pending Rewards
	/**
	 * @brief 보스 처치 보상을 런 지갑에 적립한다. 이 시점에는 확정 지급되지 않는다.
	 * 지급액은 첫 클리어 여부를 기준으로 지금 확정되며, 첫 클리어 기록도 정산 시점까지 함께 보류된다.
	 * @param RewardData 보스가 보유한 보상 정의 DataAsset (nullptr이거나 BossID/RewardCurrency가 비어있으면 무시)
	 * @param SourceBossActor 보상 대상 보스 액터 — Run 내 중복 적립 방지 키로 사용
	 * @param Multiplier 스테이지 깊이에 따른 보상 배수
	 */
	void DepositBossReward(const UACDataAsset_BossReward* RewardData, AActor* SourceBossActor, float Multiplier);

	UFUNCTION(BlueprintPure, Category = "RunState|Reward")
	int32 GetPendingCurrency(FGameplayTag CurrencyTag) const;

	// 미정산 재화 전체 — 출구 UI가 "지금 나가면 받을 금액"을 나열할 때 사용
	FORCEINLINE const TMap<FGameplayTag, int32>& GetPendingCurrencies() const { return PendingCurrencies; }
	#pragma endregion

	// 런 지갑이 변경될 때마다 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "RunState")
	FOnPendingRewardChanged OnPendingRewardChangedDelegate;

private:
	/**
	 * @brief RunDefinition을 검증해 유효한 스테이지 목록을 만든다. 상태는 변경하지 않는다.
	 *
	 * @param RunDefinition 검증할 구성
	 * @param OutStages 검증을 통과한 스테이지들이 담긴다
	 * @return 모든 검증을 통과했으면 true
	 */
	bool ValidateRunDefinition(const UACDataAsset_RunDefinition* RunDefinition, TArray<TObjectPtr<UACDataAsset_StageDefinition>>& OutStages) const;

	// 검증이 끝난 구성으로 런 상태를 교체한다. ValidateRunDefinition이 성공한 뒤에만 호출할 것
	void CommitRun(UACDataAsset_RunDefinition* RunDefinition, TArray<TObjectPtr<UACDataAsset_StageDefinition>>&& Stages);

	// 이번 스테이지 진행분(카드·적립분·클리어 플래그)만 비운다. 런 구성과 인덱스는 유지된다
	void ResetRunProgress();

	// 런 스코프 상태를 전부 비운다. 정산 여부와 무관하게 종료 경로 공통으로 호출된다.
	void ResetRunState();

	// 이번 Run에서 진행할 스테이지 구성. 레벨 전환을 넘어 유지되므로 GC로부터 보호한다
	UPROPERTY(Transient)
	TObjectPtr<UACDataAsset_RunDefinition> ActiveRunDefinition;

	// 실제로 진행할 스테이지 순서. OrderedStages의 복사본이며, 향후 랜덤 순서를 지원해도 원본 DataAsset은 변경하지 않는다
	UPROPERTY(Transient)
	TArray<TObjectPtr<UACDataAsset_StageDefinition>> RuntimeStageOrder;

	// 현재 Run이 진행 중인지 여부. 로비에서는 false다.
	bool bRunActive = false;

	// 아레나에서 바로 PIE를 시작해 열린 런인지 여부. true면 정산 시 영구 데이터를 커밋하지 않는다
	bool bDebugRun = false;

	// RuntimeStageOrder에서 현재 진행 중인 스테이지의 인덱스
	int32 CurrentStageIndex = 0;

	// 현재 스테이지의 보스를 잡았는지 여부. 다음 스테이지 진행의 전제 조건이다
	bool bCurrentStageCleared = false;

	// 이번 Run에서 클리어한 스테이지 수
	int32 ClearedBossCount = 0;

	// 이번 Run에서 획득한 카드 ID -> 중첩 수
	TMap<FName, int32> AcquiredCardStacks;

	// 이번 Run에서 전설 카드를 이미 획득했는지 여부
	bool bLegendaryUsed = false;

	// 아직 확정되지 않은 재화 적립분 (재화 태그 -> 금액)
	TMap<FGameplayTag, int32> PendingCurrencies;

	// 아직 확정되지 않은 첫 클리어 보스 태그 — 정산 시 SaveGame에 기록된다
	FGameplayTagContainer PendingClearedBossTags;

	// 이번 Run에서 이미 적립을 마친 보스 액터 — 중복 적립 방지용
	TSet<TWeakObjectPtr<AActor>> DepositedBosses;
};
