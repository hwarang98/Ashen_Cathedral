// 런 밖에서 영속되는 메타 성장 재화(성흔 조각)를 관리하는 GameInstanceSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ACMetaProgressionSubsystem.generated.h"

class UACDataAsset_BossReward;
class UACSaveGame_MetaProgression;

// 성흔 조각 보유량이 변경될 때 브로드캐스트된다. UI가 직접 구독해 표시 값을 갱신한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScarFragmentsChanged, int32, NewAmount);

/**
 * @brief 성흔 조각(메타 성장 재화) 보유량과 보스 첫 클리어 여부를 관리하는 Subsystem.
 *
 * GameInstance 생명주기를 따르므로 로비<->보스 아레나 레벨 전환(OpenLevel)에도 값이 유지된다.
 * Initialize()에서 SaveGame을 로드하고, 보상 지급 시마다 동기 저장한다.
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
	 * 슬롯에 저장된 데이터가 없으면 새 데이터로 시작한다. 로드 직후 OnScarFragmentsChangedDelegate를 브로드캐스트해 UI를 갱신시킨다.
	 * 메인 메뉴의 슬롯 선택 UI가 있다면 그곳에서 호출하는 것을 상정한다. 별도로 호출하지 않으면 Initialize()가 슬롯 0으로 로드한다.
	 * @param SlotIndex 로드할 슬롯 번호 (0 이상)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void LoadSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	FORCEINLINE int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	/**
	 * @brief 보스 처치 보상을 지급한다. 첫 클리어/반복 클리어 여부에 따라 금액이 달라진다.
	 * 같은 보스 액터 인스턴스에 대해 세션 내 중복 호출되어도 한 번만 지급된다.
	 * @param RewardData 보스가 보유한 보상 정의 DataAsset (nullptr이거나 BossID가 비어있으면 무시)
	 * @param SourceBossActor 보상 대상 보스 액터 — 세션 내 중복 지급 방지 키로 사용
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void GrantBossReward(const UACDataAsset_BossReward* RewardData, AActor* SourceBossActor);

	/**
	 * @brief 성흔 조각을 증감하고 저장·델리게이트 브로드캐스트까지 함께 처리한다.
	 * @param Amount 증감량 (음수 가능 — 추후 소비 로직에서 재사용)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void AddScarFragments(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 GetScarFragments() const;

	// BossID가 이미 첫 클리어를 완료했는지 여부
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	bool IsBossCleared(FGameplayTag BossID) const;

	// 성흔 조각 보유량이 변경될 때마다 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "MetaProgression")
	FOnScarFragmentsChanged OnScarFragmentsChangedDelegate;

private:
	void SaveProgress();

	// 슬롯 번호로부터 실제 SaveGameToSlot/LoadGameFromSlot에 쓸 슬롯 이름을 만든다 (예: 0 -> "MetaProgressionSave_Slot0")
	static FString BuildSaveSlotName(int32 SlotIndex);

	UPROPERTY()
	TObjectPtr<UACSaveGame_MetaProgression> SaveGameInstance;

	// 세션 내 이미 보상을 지급한 보스 액터 — 저장하지 않는 휘발성 중복 방지용
	TSet<TWeakObjectPtr<AActor>> GrantedThisSession;

	// 현재 로드되어 있는 슬롯 번호. 슬롯 선택 UI가 아직 없어 기본값 0을 사용한다.
	int32 ActiveSlotIndex = 0;

	static const FString SaveSlotBaseName;
	static const int32 SaveUserIndex;
};