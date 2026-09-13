// 런 밖에서 영속되는 메타 성장 재화를 관리하는 GameInstanceSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "SaveGame/ACSavedInventoryEntry.h"
#include "ACMetaProgressionSubsystem.generated.h"

class UACDataAsset_BossReward;
class UACSaveGame_MetaProgression;

// 재화 보유량이 변경될 때 브로드캐스트된다. UI가 CurrencyTag로 분기해 표시 값을 갱신한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrencyChanged, FGameplayTag, CurrencyTag, int32, NewAmount);

// 활성 슬롯이 바뀌거나(로드) 비워질 때(삭제) 브로드캐스트된다. 슬롯 데이터를 캐시하는 쪽이 다시 읽는 신호다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveSlotChanged, int32, NewSlotIndex);

/**
 * @brief 메타 성장 재화 보유량, 보스 첫 클리어 여부, 슬롯별 영속 데이터를 관리하는 Subsystem.
 *
 * GameInstance 생명주기를 따르므로 로비<->보스 아레나 레벨 전환(OpenLevel)에도 값이 유지된다.
 * GameInstance 클래스와 무관하게 부착되므로 UGT 템플릿의 GameInstance를 써도 그대로 동작한다.
 *
 * Initialize()는 어떤 슬롯도 로드하지 않는다. UGT 슬롯 메뉴가 슬롯을 확정하고
 * BP_ACGameInstance::SyncMetaProgressionSlot -> LoadSlot()이 호출되기 전까지는
 * 메모리 전용(transient) 데이터로만 동작하며 SaveGame 파일을 만들지도 바꾸지도 않는다.
 *
 * 인벤토리 데이터(InventoryEntries / EquippedItemDefinitionId)도 같은 슬롯 파일에 들어간다.
 * 실제 인벤토리 API는 UACInventorySubsystem이 제공하고, 이 클래스는 저장소와 디스크 I/O만 소유한다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACMetaProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * @brief 지정한 슬롯 번호의 세이브 데이터를 로드해 활성 슬롯으로 전환한다.
	 * 슬롯에 저장된 데이터가 없으면 새 데이터로 시작한다. 로드 직후 전 재화의 변경을 브로드캐스트해 UI를 갱신시킨다.
	 * UGT 슬롯 메뉴에서 슬롯 번호가 확정될 때 BP_ACGameInstance를 통해 호출하는 것을 상정한다.
	 * 이미 활성인 슬롯을 다시 요청하면 아무것도 하지 않는다 — 레벨 전환마다 디스크를 다시 읽어 진행이 되돌아가는 것을 막는다.
	 * 다른 슬롯으로 바꿀 때는 이전 슬롯의 미저장 변경을 먼저 flush한다.
	 * @param SlotIndex 로드할 슬롯 번호 (0 이상. 음수는 거부되고 슬롯 미선택 상태가 유지된다)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void LoadSlot(int32 SlotIndex);

	/**
	 * @brief 지정한 슬롯의 세이브 파일을 삭제한다. UGT의 '새 게임' / 슬롯 삭제와 함께 호출해야 한다.
	 * 호출하지 않으면 새 게임을 시작해도 이전 재화와 인벤토리가 그대로 남는다.
	 * 활성 슬롯을 삭제한 경우 슬롯 미선택 상태로 되돌아간다 — 삭제한 슬롯이 이후 flush로 되살아날 경로를 없앤다.
	 * 파일이 실제로 존재하는데 삭제에 실패하면 메모리 상태를 건드리지 않는다. 지워지지도 않은 슬롯을 비운 것처럼 보이면 안 되기 때문이다.
	 * 애초에 파일이 없던 신규 슬롯은 정상적인 빈 슬롯으로 취급한다.
	 * @param SlotIndex 삭제할 슬롯 번호 (0 이상. 음수는 거부된다)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void DeleteSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	FORCEINLINE int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	// 슬롯이 확정되어 실제 파일에 저장할 수 있는 상태인지 여부. false면 모든 변경이 메모리에만 남는다
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	FORCEINLINE bool HasActiveSlot() const { return bHasActiveSlot; }

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

	#pragma region Inventory Storage
	// 저장된 인벤토리 항목. UACInventorySubsystem이 로드 직후 런타임 표현을 만드는 원본이다
	const TArray<FACSavedInventoryEntry>& GetSavedInventoryEntries() const;

	/**
	 * @brief 인벤토리 항목 전체를 교체한다. 저장은 하지 않으므로 호출자가 RequestSave()로 마무리해야 한다.
	 * UACInventorySubsystem의 단일 커밋 경로에서만 호출한다.
	 * @param InEntries 런타임 인벤토리로부터 만들어진 저장용 항목 배열
	 */
	void SetSavedInventoryEntries(TArray<FACSavedInventoryEntry>&& InEntries);

	// 마지막으로 장착한 아이템 정의 ID. 장착 이력이 없으면 무효한 ID
	FPrimaryAssetId GetEquippedItemDefinitionId() const;

	/**
	 * @brief 장착 아이템 정의 ID를 기록한다. 저장은 하지 않는다. 무효한 ID를 넣으면 장착 해제로 취급한다.
	 * @param InItemDefinitionId 기록할 정의 ID
	 * @return 값이 실제로 바뀌었으면 true
	 */
	bool SetEquippedItemDefinitionId(const FPrimaryAssetId& InItemDefinitionId);

	/**
	 * @brief 변경을 dirty로 표시하고 활성 슬롯 파일에 기록한다.
	 * 활성 슬롯이 없거나 저장이 막혀 있거나 쓰기에 실패하면 dirty 상태가 유지되어,
	 * 슬롯 전환·종료 시 FlushPendingSave()가 다시 시도한다.
	 * @return 실제로 디스크에 기록했으면 true
	 */
	bool RequestSave();

	// 아직 디스크에 반영되지 않은 변경이 남아 있는지 여부. 저장 실패 후 재시도 대상이 있다는 뜻이다
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	FORCEINLINE bool HasPendingSave() const { return bPendingSave; }

	/**
	 * 쓰기에 실패한 채 슬롯이 바뀌어 재시도를 기다리는 슬롯 수.
	 * 0이 아니면 아직 디스크에 못 올라간 다른 슬롯의 데이터가 메모리에 남아 있다는 뜻이다.
	 */
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	FORCEINLINE int32 GetPendingSlotWriteCount() const { return PendingSlotWrites.Num(); }
	#pragma endregion

	// 재화 보유량이 변경될 때마다 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "MetaProgression")
	FOnCurrencyChanged OnCurrencyChangedDelegate;

	// 활성 슬롯이 바뀌거나 비워질 때 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "MetaProgression")
	FOnActiveSlotChanged OnActiveSlotChangedDelegate;

	#if WITH_EDITOR
	/**
	 * @brief 자동화 테스트가 실제 세이브 파일을 건드리지 않도록 슬롯 이름 접두사를 강제한다.
	 * 빈 문자열을 넣으면 원래 규칙(PIE 샌드박스 포함)으로 돌아간다.
	 * @param InPrefix 슬롯 이름 앞에 붙일 접두사 (예: "AutoTest_")
	 */
	static void SetSlotNamePrefixOverrideForTests(const FString& InPrefix);

	/**
	 * @brief 자동화 테스트가 UGameInstance::Init() 없이 이 서브시스템을 쓰도록 초기 상태만 만든다.
	 * Init()은 전역 델리게이트를 바인딩하고 다른 서브시스템까지 전부 깨우므로 테스트에서 부를 수 없다.
	 */
	void InitializeForTests();

	/**
	 * @brief 자동화 테스트가 저장 실패 경로를 재현하도록 실제 쓰기를 건너뛰고 실패로 처리한다.
	 * 파일명 규칙 같은 OS 의존 트릭 없이 결정적으로 실패시키기 위한 훅이다.
	 * @param bInForceFailure true면 SaveProgress()가 항상 실패한다
	 */
	static void SetForceSaveFailureForTests(bool bInForceFailure);
	#endif

private:
	/**
	 * @brief 실제 디스크 쓰기. 성공했을 때만 dirty 플래그를 내린다.
	 * 활성 슬롯이 없거나 저장이 막혀 있으면 경고만 남기고 아무것도 쓰지 않는다.
	 * @return 실제로 기록했으면 true
	 */
	bool SaveProgress();

	// 변경을 dirty로 표시한 뒤 저장을 시도한다. 재화/보스/인벤토리/장착 변경이 모두 이 경로를 지난다
	void MarkDirtyAndSave();

	/**
	 * @brief 미저장 변경이 남아 있으면 현재 활성 슬롯에 기록한다. 슬롯 전환과 종료 시점의 재시도 지점이다.
	 * @return 쓸 것이 없었거나 성공적으로 기록했으면 true. 실패했으면 false
	 */
	bool FlushPendingSave();

	/**
	 * @brief 쓰기에 실패한 현재 슬롯 데이터를 보류 큐로 옮긴다. 슬롯을 바꾸기 직전에만 호출한다.
	 * 이렇게 하지 않으면 SaveGameInstance가 교체되면서 미저장 변경이 통째로 사라진다.
	 */
	void StashPendingWriteForRetry();

	// 보류 큐에 쌓인 실패한 쓰기를 다시 시도한다. 성공한 항목만 큐에서 빠진다
	void RetryPendingSlotWrites();

	/**
	 * @brief 실제 SaveGameToSlot 호출 지점. 테스트용 강제 실패 훅도 여기 한 곳에서만 판정한다.
	 * @param InSaveGame 기록할 세이브 객체
	 * @param InSlotName 접두사까지 포함된 최종 슬롯 이름
	 * @return 기록에 성공했으면 true
	 */
	bool WriteSaveGameToSlot(UACSaveGame_MetaProgression* InSaveGame, const FString& InSlotName) const;

	// 슬롯 로드/삭제 직후 UI를 일괄 갱신하기 위해 전 재화의 현재 값을 브로드캐스트한다
	void BroadcastAllCurrencies();

	// 슬롯이 없을 때 쓰는 메모리 전용 세이브 객체를 만든다. 파일은 만들지 않는다
	void ResetToTransientSaveData();

	// 슬롯 번호로부터 실제 SaveGameToSlot/LoadGameFromSlot에 쓸 슬롯 이름을 만든다 (예: 0 -> "MetaProgressionSave_Slot_0")
	FString BuildSaveSlotName(int32 SlotIndex) const;

	/**
	 * @brief 슬롯 이름 앞에 붙일 접두사를 결정한다.
	 * PIE에서는 실제 세이브를 절대 건드리지 않도록 "PIE_"를 붙인다 (ac.MetaProgression.PIESandbox 0으로 끌 수 있음).
	 * @return 접두사. 실제 슬롯을 쓸 때는 빈 문자열
	 */
	FString ResolveSaveSlotPrefix() const;

	UPROPERTY()
	TObjectPtr<UACSaveGame_MetaProgression> SaveGameInstance;

	/**
	 * 쓰기에 실패한 채 슬롯이 바뀐 데이터를 슬롯 이름별로 보관한다 (같은 슬롯은 최신 것 하나만).
	 * 디스크 꽉 참·권한·파일 잠금은 대개 일시적이라, 나중에 저장이 성공할 때 원래 슬롯으로 올려보낸다.
	 * UPROPERTY라야 GC가 가져가지 않는다. 슬롯을 삭제하면 그 슬롯의 보류분도 함께 버린다.
	 */
	UPROPERTY()
	TMap<FString, TObjectPtr<UACSaveGame_MetaProgression>> PendingSlotWrites;

	// 세션 내 이미 보상을 지급한 보스 액터 — 저장하지 않는 휘발성 중복 방지용
	TSet<TWeakObjectPtr<AActor>> GrantedThisSession;

	// 현재 로드되어 있는 슬롯 번호. UGT 슬롯 메뉴가 확정하기 전까지는 INDEX_NONE이다.
	int32 ActiveSlotIndex = INDEX_NONE;

	// 슬롯이 확정되었는지 여부. false인 동안에는 어떤 SaveGame 파일도 만들지 않는다
	bool bHasActiveSlot = false;

	// 아직 디스크에 기록되지 않은 변경이 있는지 여부. 슬롯 전환/종료 시 flush 대상이다
	bool bPendingSave = false;

	// 슬롯 없이 저장을 시도했다는 경고를 슬롯 상태가 바뀔 때까지 한 번만 남기기 위한 플래그
	bool bWarnedSaveWithoutSlot = false;

	// 손상된 파일이거나 이 빌드보다 새 버전이라 덮어쓰면 안 되는 슬롯인지 여부. true면 읽기만 한다
	bool bSaveBlocked = false;

	// 저장이 막혔다는 경고를 슬롯 상태가 바뀔 때까지 한 번만 남기기 위한 플래그
	bool bWarnedSaveBlocked = false;

	// 슬롯을 활성화한 시점에 정해진 접두사. 로드와 저장이 서로 다른 파일을 가리키지 않도록 고정해 둔다
	FString ActiveSlotPrefix;

	static const FString SaveSlotBaseName;
	static const int32 SaveUserIndex;

	#if WITH_EDITOR
	// 테스트가 지정한 슬롯 이름 접두사. 비어 있지 않으면 PIE 판정보다 우선한다
	static FString SlotNamePrefixOverrideForTests;

	// true면 SaveProgress()가 실제 쓰기 없이 실패한다 (저장 실패 경로 테스트용)
	static bool bForceSaveFailureForTests;
	#endif
};
