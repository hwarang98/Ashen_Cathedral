// 런 밖에서 영속되는 메타 성장 재화를 관리하는 GameInstanceSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ACMetaProgressionSubsystem.generated.h"

class UACDataAsset_BossReward;
class UACSaveGame_MetaProgression;

// 재화 보유량이 변경될 때 브로드캐스트된다. UI가 CurrencyTag로 분기해 표시 값을 갱신한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrencyChanged, FGameplayTag, CurrencyTag, int32, NewAmount);

/**
 * @brief 메타 성장 재화 보유량과 보스 첫 클리어 여부를 관리하는 Subsystem.
 *
 * GameInstance 생명주기를 따르므로 로비<->보스 아레나 레벨 전환(OpenLevel)에도 값이 유지된다.
 * GameInstance 클래스와 무관하게 부착되므로 UGT 템플릿의 GameInstance를 써도 그대로 동작한다.
 * Initialize()에서 SaveGame을 로드하고, 재화가 바뀔 때마다 동기 저장한다.
 * 보스 사망 시 AACGameMode::HandleBossBattleCompleted가 GrantBossReward()를 호출해 보상을 지급한다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACMetaProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * @brief 지정한 슬롯 번호의 세이브 데이터를 로드해 활성 슬롯으로 전환한다.
	 * 슬롯에 저장된 데이터가 없으면 새 데이터로 시작한다. 로드 직후 전 재화의 변경을 브로드캐스트해 UI를 갱신시킨다.
	 * UGT 슬롯 메뉴에서 슬롯 번호가 확정될 때 BP_ACGameInstance를 통해 호출하는 것을 상정한다.
	 * 별도로 호출하지 않으면 Initialize()가 슬롯 0으로 로드한다.
	 * @param SlotIndex 로드할 슬롯 번호 (0 이상)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void LoadSlot(int32 SlotIndex);

	/**
	 * @brief 지정한 슬롯의 세이브 파일을 삭제한다. UGT의 '새 게임' / 슬롯 삭제와 함께 호출해야 한다.
	 * 호출하지 않으면 새 게임을 시작해도 이전 재화가 그대로 남는다.
	 * 활성 슬롯을 삭제한 경우 빈 데이터로 재초기화하고 전 재화의 변경을 브로드캐스트한다.
	 * @param SlotIndex 삭제할 슬롯 번호 (0 이상)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void DeleteSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	FORCEINLINE int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	/**
	 * @brief 보스 처치 보상을 지급한다. 첫 클리어/반복 클리어 여부에 따라 금액이 달라진다.
	 * 같은 보스 액터 인스턴스에 대해 세션 내 중복 호출되어도 한 번만 지급된다.
	 * @param RewardData 보스가 보유한 보상 정의 DataAsset (nullptr이거나 BossID/RewardCurrency가 비어있으면 무시)
	 * @param SourceBossActor 보상 대상 보스 액터 — 세션 내 중복 지급 방지 키로 사용
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void GrantBossReward(const UACDataAsset_BossReward* RewardData, AActor* SourceBossActor);

	/**
	 * @brief 보스 처치 시 지급될 재화량만 계산해 반환한다. 지급도, 클리어 기록도 하지 않는다.
	 * UACRunStateSubsystem이 런 지갑에 적립할 금액을 잡은 시점에 확정하기 위해 사용한다.
	 * @param RewardData 보스가 보유한 보상 정의 DataAsset
	 * @param bFirstClear 첫 클리어로 취급할지 여부 — 정산 대기 중인 클리어까지 감안해 호출자가 판단한다
	 * @return 지급 예정 재화량. RewardData가 유효하지 않으면 0
	 */
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 EvaluateBossRewardAmount(const UACDataAsset_BossReward* RewardData, bool bFirstClear) const;

	/**
	 * @brief BossID를 첫 클리어 완료로 기록하고 저장한다.
	 * 런 정산(UACRunStateSubsystem::SettleAndEndRun) 시점에 보류돼 있던 클리어를 확정할 때 호출한다.
	 * @param BossID 기록할 보스 태그
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void MarkBossCleared(FGameplayTag BossID);

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 GetCurrencyAmount(FGameplayTag CurrencyTag) const;

	/**
	 * @brief 재화를 증감하고 저장·델리게이트 브로드캐스트까지 함께 처리한다.
	 * 재화가 바뀌는 모든 경로는 이 함수를 거쳐야 저장 누락이 발생하지 않는다.
	 * @param CurrencyTag 대상 재화 (MetaProgression.Currency.*)
	 * @param Amount 증감량 (음수 가능). 결과가 0 미만이면 0으로 클램프된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void AddCurrency(FGameplayTag CurrencyTag, int32 Amount);

	// 해당 재화를 Amount만큼 소비할 수 있는지 여부. Amount가 0 이하면 false
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	bool CanSpendCurrency(FGameplayTag CurrencyTag, int32 Amount) const;

	/**
	 * @brief 재화를 소비한다. 잔액이 부족하면 아무것도 바꾸지 않는다.
	 * @param CurrencyTag 대상 재화 (MetaProgression.Currency.*)
	 * @param Amount 소비할 양 (양수)
	 * @return 실제로 소비했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	bool SpendCurrency(FGameplayTag CurrencyTag, int32 Amount);

	// 정의된 전체 재화 태그 목록. UI가 재화 슬롯을 순회 생성할 때 사용한다.
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	static TArray<FGameplayTag> GetAllCurrencyTags();

	// BossID가 이미 첫 클리어를 완료했는지 여부
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	bool IsBossCleared(FGameplayTag BossID) const;

	// 재화 보유량이 변경될 때마다 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "MetaProgression")
	FOnCurrencyChanged OnCurrencyChangedDelegate;

private:
	void SaveProgress();

	// 슬롯 로드/삭제 직후 UI를 일괄 갱신하기 위해 전 재화의 현재 값을 브로드캐스트한다
	void BroadcastAllCurrencies();

	// 슬롯 번호로부터 실제 SaveGameToSlot/LoadGameFromSlot에 쓸 슬롯 이름을 만든다 (예: 0 -> "MetaProgressionSave_Slot_0")
	static FString BuildSaveSlotName(int32 SlotIndex);

	UPROPERTY()
	TObjectPtr<UACSaveGame_MetaProgression> SaveGameInstance;

	// 세션 내 이미 보상을 지급한 보스 액터 — 저장하지 않는 휘발성 중복 방지용
	TSet<TWeakObjectPtr<AActor>> GrantedThisSession;

	// 현재 로드되어 있는 슬롯 번호. UGT 슬롯 메뉴가 확정하기 전까지는 기본값 0을 사용한다.
	int32 ActiveSlotIndex = 0;

	static const FString SaveSlotBaseName;
	static const int32 SaveUserIndex;
};
