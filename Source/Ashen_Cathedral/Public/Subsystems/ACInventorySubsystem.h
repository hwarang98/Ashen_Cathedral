// 슬롯에 영속되는 인벤토리를 관리하는 GameInstanceSubsystem — 저장소와 디스크 I/O는 UACMetaProgressionSubsystem이 소유한다

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "Structs/ACInventoryEntry.h"
#include "SaveGame/ACSavedInventoryEntry.h"
#include "ACInventorySubsystem.generated.h"

class UACItemDefinition;
class UACMetaProgressionSubsystem;

// 특정 아이템의 총 보유 수량이 바뀔 때마다 브로드캐스트된다. 수량이 0이 되면 NewTotalCount가 0으로 온다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, UACItemDefinition*, ItemDefinition, int32, NewTotalCount);

// 장착 아이템이 바뀔 때 브로드캐스트된다. 장착 해제 시 nullptr이 온다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChanged, UACItemDefinition*, NewEquippedItem);

/**
 * @brief 슬롯 단위로 영속되는 인벤토리의 단일 진입점.
 *
 * GameInstance 생명주기를 따르므로 로비<->보스 아레나 레벨 전환(OpenLevel)에도 내용이 유지된다.
 * 실제 저장은 UACMetaProgressionSubsystem이 소유한 UACSaveGame_MetaProgression에 함께 들어가므로,
 * UGT의 슬롯 삭제(BP_ACGameInstance::DeleteMetaProgressionSlot)가 재화/보스 기록/인벤토리를 한 번에 지운다.
 *
 * 인벤토리를 바꾸는 모든 경로(AddItem/RemoveItem/SetStackCount/EquipItem)는
 * CommitInventoryChange()라는 한 지점을 통과해 저장용 구조체 갱신 -> 델리게이트 -> 저장 순서를 보장한다.
 * UI나 Tick에서 직접 저장하지 않는다.
 *
 * 런 전용 상태(획득 카드, 미정산 재화, 현재 스테이지, 임시 버프)는 여기에 들어오지 않는다 — UACRunStateSubsystem 소관이다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * @brief 아이템을 추가한다. 스택 상한을 넘으면 새 항목을 만들어 나눠 담는다.
	 * @param ItemDefinition 추가할 아이템 원형 (nullptr이면 아무것도 하지 않는다)
	 * @param Count 추가할 수량 (1 이상)
	 * @return 실제로 추가된 수량
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UACItemDefinition* ItemDefinition, int32 Count = 1);

	/**
	 * @brief 아이템을 제거한다. 수량이 부족하면 있는 만큼만 제거한다.
	 * 여러 항목에 나뉘어 있으면 뒤쪽 항목부터 비운다 — 먼저 얻은 항목의 강화 상태를 최대한 보존하기 위해서다.
	 * @param ItemDefinition 제거할 아이템 원형
	 * @param Count 제거할 수량 (1 이상)
	 * @return 실제로 제거된 수량
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(UACItemDefinition* ItemDefinition, int32 Count = 1);

	/**
	 * @brief 특정 항목의 수량을 직접 지정한다. 0을 넣으면 항목이 제거된다.
	 * @param InstanceId 대상 항목의 InstanceId
	 * @param NewStackCount 새 수량 (스택 상한으로 잘린다)
	 * @return 항목을 찾아 반영했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetStackCount(FGuid InstanceId, int32 NewStackCount);

	/**
	 * @brief 항목의 강화 단계를 설정한다.
	 * @param InstanceId 대상 항목의 InstanceId
	 * @param NewUpgradeLevel 새 강화 단계 (0 이상)
	 * @return 항목을 찾아 반영했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetUpgradeLevel(FGuid InstanceId, int32 NewUpgradeLevel);

	/**
	 * @brief 항목의 수치 스택을 증감한다. 결과가 0 이하가 되면 해당 키를 제거한다.
	 * @param InstanceId 대상 항목의 InstanceId
	 * @param StatTag 수치 키 (Item.Stat.*)
	 * @param DeltaCount 증감량
	 * @return 항목을 찾아 반영했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddStatTagStack(FGuid InstanceId, FGameplayTag StatTag, int32 DeltaCount);

	/**
	 * @brief 마지막으로 선택한 아이템을 기록한다. 실제 무기 스폰/장착은 하지 않는다.
	 * 보유하지 않은 아이템은 거부된다. nullptr을 넣으면 장착 해제로 기록한다.
	 * @param ItemDefinition 장착으로 기록할 아이템 원형
	 * @return 기록했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipItem(UACItemDefinition* ItemDefinition);

	// 장착 기록을 지운다
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearEquippedItem();

	// 장착으로 기록된 아이템. 해석에 실패하거나 기록이 없으면 nullptr
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UACItemDefinition* GetEquippedItemDefinition() const;

	// 장착으로 기록된 아이템의 정의 ID. 기록이 없으면 무효한 ID
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FPrimaryAssetId GetEquippedItemDefinitionId() const;

	// 현재 인벤토리 항목 전체 (C++ 전용 — 복사 없이 훑을 때 쓴다)
	FORCEINLINE const TArray<FACInventoryEntry>& GetEntries() const { return Entries; }

	// 현재 인벤토리 항목 전체의 복사본. 블루프린트/UI 순회용
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FACInventoryEntry> GetAllEntries() const { return Entries; }

	// 분류 태그가 일치하는(하위 태그 포함) 항목만 추려 반환한다
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FACInventoryEntry> GetEntriesByCategory(FGameplayTag CategoryTag) const;

	// 해당 아이템의 총 보유 수량 (여러 항목에 나뉘어 있으면 합산)
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalItemCount(const UACItemDefinition* ItemDefinition) const;

	// 해당 아이템을 하나라도 보유하고 있는지 여부
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(const UACItemDefinition* ItemDefinition) const;

	// InstanceId로 항목을 찾는다. 없으면 nullptr
	const FACInventoryEntry* FindEntryByInstanceId(const FGuid& InstanceId) const;

	// 아이템 총 보유 수량이 바뀔 때마다 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChangedDelegate;

	// 장착 기록이 바뀔 때 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnEquippedItemChanged OnEquippedItemChangedDelegate;

	#if WITH_EDITOR
	/**
	 * @brief 자동화 테스트가 AssetManager 없이 ItemDefinition을 해석하도록 리졸버를 갈아끼운다.
	 * 테스트용 정의는 실제 에셋이 아니라 AssetManager가 알지 못하므로 이 훅이 없으면 전부 해석에 실패한다.
	 * @param InResolver ID를 정의로 바꿔주는 함수. 비우면 기본 AssetManager 경로로 돌아간다
	 */
	static void SetItemDefinitionResolverForTests(TFunction<UACItemDefinition*(const FPrimaryAssetId&)> InResolver);

	/**
	 * @brief 자동화 테스트가 UGameInstance::Init() 없이 쓰도록 의존 서브시스템을 직접 주입한다.
	 * Init()을 거치지 않으면 GameInstance의 서브시스템 컬렉션이 비어 있어 GetSubsystem()이 항상 null이다.
	 * @param InMetaProgression 저장소 역할을 할 메타 진행 서브시스템
	 */
	void InitializeForTests(UACMetaProgressionSubsystem* InMetaProgression);
	#endif

private:
	// 활성 슬롯이 바뀔 때 저장 데이터로부터 런타임 항목을 다시 만든다
	UFUNCTION()
	void HandleActiveSlotChanged(int32 NewSlotIndex);

	/**
	 * @brief 런타임 변경을 확정하는 단일 경로. 저장용 구조체 갱신 -> 델리게이트 -> 저장 순으로 처리한다.
	 * @param ChangedDefinitions 수량이 바뀐 아이템 원형 목록 (중복 없이)
	 */
	void CommitInventoryChange(const TArray<UACItemDefinition*>& ChangedDefinitions);

	// 저장 데이터로부터 Entries를 다시 만든다. 해석되지 않는 항목은 건너뛰고 경고만 남긴다
	void RebuildEntriesFromSaveData();

	/**
	 * @brief 보유량이 0이 된 아이템이 장착 기록과 같으면 기록을 비우고 통지한다.
	 * 저장은 하지 않는다 — 호출자의 CommitInventoryChange 한 번에 묶어 중복 기록을 피한다.
	 * @param ItemDefinition 방금 수량이 줄어든 아이템 원형
	 */
	void ClearEquippedIfNoLongerOwned(const UACItemDefinition* ItemDefinition);

	/**
	 * @brief 정의별 총 보유량 맵을 만든다. 슬롯 전환 전후를 비교해 변경 통지를 만드는 데 쓴다.
	 * @param OutTotals 정의 -> 총 수량
	 */
	void GatherTotalsByDefinition(TMap<UACItemDefinition*, int32>& OutTotals) const;

	// 런타임 항목을 저장용 구조체 배열로 변환한다
	TArray<FACSavedInventoryEntry> BuildSavedEntries() const;

	// FPrimaryAssetId를 실제 정의 에셋으로 해석한다. 실패하면 nullptr
	static UACItemDefinition* ResolveItemDefinition(const FPrimaryAssetId& ItemDefinitionId);

	UACMetaProgressionSubsystem* GetMetaProgression() const;

	UPROPERTY()
	TArray<FACInventoryEntry> Entries;

	/**
	 * ID는 멀쩡한데 지금 이 실행에서 정의 에셋을 찾지 못한 항목들.
	 * 런타임 인벤토리에는 넣지 않지만 저장할 때 그대로 되돌려 쓴다.
	 * 에셋이 잠시 빠졌거나(플러그인 미로드, AssetManager 스캔 미완료) 하는 상황에서
	 * 다음 저장 한 번으로 플레이어의 아이템이 영구히 사라지는 것을 막는다.
	 */
	TArray<FACSavedInventoryEntry> UnresolvedSavedEntries;

	#if WITH_EDITOR
	// 테스트가 지정한 정의 리졸버. 비어 있지 않으면 AssetManager보다 우선한다
	static TFunction<UACItemDefinition*(const FPrimaryAssetId&)> ItemDefinitionResolverForTests;

	// 테스트가 주입한 메타 진행 서브시스템. 유효하면 GameInstance의 컬렉션보다 우선한다
	TWeakObjectPtr<UACMetaProgressionSubsystem> MetaProgressionForTests;
	#endif
};
