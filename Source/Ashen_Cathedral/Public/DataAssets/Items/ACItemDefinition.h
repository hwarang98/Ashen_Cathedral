// 인벤토리 아이템 원형(불변 데이터)을 정의하는 PrimaryDataAsset — 런타임 보유 수량과 상태는 UACInventorySubsystem이 소유한다

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ACItemDefinition.generated.h"

class UACDataAsset_WeaponData;
class UTexture2D;

/**
 * @brief 아이템 한 종류의 불변 정의. SaveGame에는 이 에셋 자체가 아니라 GetPrimaryAssetId()만 기록된다.
 *
 * PrimaryAssetType은 하위 클래스를 만들어도 항상 "ACItemDefinition"으로 고정된다.
 * 타입이 클래스명을 따라가면 C++ 파생 클래스를 하나 추가하는 순간 기존 세이브의 ItemDefinitionId가 해석되지 않기 때문이다.
 * 따라서 AssetManager 스캔 규칙(Config/DefaultGame.ini의 PrimaryAssetTypesToScan)도 이 타입 하나만 등록하면 된다.
 *
 * 무기 아이템은 기존 UACDataAsset_WeaponData를 대체하지 않고 SoftWeaponData로 참조만 한다.
 * 무기 스폰/장착 오케스트레이션은 지금까지대로 UPlayerCombatComponent와 UACWeaponSelectionSubsystem이 담당한다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * @brief 세이브에 기록될 안정적인 식별자. 타입은 항상 ItemDefinitionAssetType, 이름은 에셋 이름이다.
	 * 에셋 이름을 바꾸면 기존 세이브의 해당 아이템은 해석되지 않고 건너뛰어진다 — 출시 후에는 이름을 바꾸지 말 것.
	 * @return 이 아이템 정의의 PrimaryAssetId
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// 스택 상한을 넘지 않는 유효한 수량으로 잘라 반환한다
	int32 ClampStackCount(int32 DesiredCount) const;

	// 실제로 적용되는 스택 상한. ClampMin 메타는 에디터 UI만 제약하므로 손으로 고친 데이터의 0/음수를 여기서 1로 막는다
	FORCEINLINE int32 GetEffectiveMaxStackCount() const { return FMath::Max(1, MaxStackCount); }

	// UI와 로그에 표시할 이름. 비어 있으면 에셋 이름이 대신 쓰인다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	// 아이템 분류. UI 필터와 인벤토리 조회의 기준이 된다 (Item.Category.*)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (Categories = "Item.Category"))
	FGameplayTag ItemCategoryTag;

	/**
	 * 한 Entry가 가질 수 있는 최대 수량. 1이면 스택되지 않고 획득할 때마다 별도 Entry가 생긴다.
	 * 총 보유량은 UACInventorySubsystem::GetTotalItemCount()가 여러 Entry를 합산해 알려준다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackCount = 1;

	// 인벤토리 슬롯 아이콘. UI가 필요할 때 비동기 로드하도록 소프트 참조로 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> SoftIconTexture;

	// 무기 아이템일 때 연결할 기존 무기 데이터. 무기가 아니면 비워 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Weapon")
	TSoftObjectPtr<UACDataAsset_WeaponData> SoftWeaponData;

	// 모든 UACItemDefinition 에셋이 공유하는 PrimaryAssetType. AssetManager 스캔 규칙의 키와 같아야 한다
	static const FPrimaryAssetType ItemDefinitionAssetType;
};
