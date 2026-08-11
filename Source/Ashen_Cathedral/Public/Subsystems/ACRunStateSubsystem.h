// 한 Run 동안만 유지되는 상태(보상 카드·보스 진행도·미정산 보상)를 레벨 전환 너머로 보관하는 GameInstanceSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ACRunStateSubsystem.generated.h"

class UACDataAsset_BossReward;

// 미정산 보상(런 지갑)이 변경될 때 브로드캐스트된다. 출구 프롬프트 등이 "지금 나가면 받을 금액"을 표시할 때 구독한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPendingRewardChanged, FGameplayTag, CurrencyTag, int32, PendingAmount);

/**
 * @brief 한 Run에 속하는 휘발성 상태를 보관하는 Subsystem.
 *
 * GameInstance 생명주기를 따르므로 아레나 -> 아레나 레벨 전환(OpenLevel)에도 값이 유지된다.
 * 플레이어 캐릭터와 GameMode는 레벨 전환 때 파괴되므로, 레벨을 넘어야 하는 런 상태는 모두 여기에 둔다.
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
	 * @brief 새 Run을 시작한다 — 카드·보스 진행도·런 지갑을 모두 초기화한다.
	 * 로비에서 아레나로 나갈 때(RequestStartRun) 호출하며, 아레나에서 바로 PIE를 시작한 경우
	 * AACGameMode::StartPlay가 대신 호출해 준다.
	 */
	UFUNCTION(BlueprintCallable, Category = "RunState")
	void BeginRun();

	/**
	 * @brief Run을 정산하고 종료한다 — 런 지갑의 재화와 첫 클리어 기록을 메타 성장에 확정 반영한다.
	 * 로비 복귀 출구와 최종 보스 클리어 경로에서 호출한다.
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

	#pragma region Boss Sequence
	// 다음에 스폰할 보스의 NextBossSequence 인덱스
	UFUNCTION(BlueprintPure, Category = "RunState|Boss")
	FORCEINLINE int32 GetNextBossIndex() const { return NextBossIndex; }

	// 현재 인덱스를 반환하고 다음 보스로 진행시킨다
	int32 ConsumeNextBossIndex();

	// 이번 Run에서 지금까지 클리어한 보스 수 — 스테이지 보상 배수의 인덱스로 쓰인다
	UFUNCTION(BlueprintPure, Category = "RunState|Boss")
	FORCEINLINE int32 GetClearedBossCount() const { return ClearedBossCount; }
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
	// 런 스코프 상태를 전부 비운다. 정산 여부와 무관하게 종료 경로 공통으로 호출된다.
	void ResetRunState();

	// 현재 Run이 진행 중인지 여부. 로비에서는 false다.
	bool bRunActive = false;

	// 다음에 스폰할 보스의 NextBossSequence 인덱스
	int32 NextBossIndex = 0;

	// 이번 Run에서 클리어한 보스 수
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
