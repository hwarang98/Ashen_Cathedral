// 메타 성장 재화와 첫 클리어 보스 목록을 보관하는 SaveGame — 프로젝트 최초의 영속 저장 데이터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "SaveGame/ACSavedInventoryEntry.h"
#include "ACSaveGame_MetaProgression.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UACSaveGame_MetaProgression : public USaveGame
{
	GENERATED_BODY()

public:
	// 이 빌드가 기록하는 세이브 구조 버전. 저장 직전에 SaveVersion에 찍는다
	static constexpr int32 CurrentSaveVersion = 2;

	/**
	 * @brief 옛 버전으로 저장된 데이터를 현재 버전 구조로 끌어올린다. 저장은 하지 않는다.
	 * 버전 1에는 인벤토리 필드가 없었으므로 빈 배열/무효 ID 그대로 두고 버전만 올린다 — 재화와 보스 기록은 손대지 않는다.
	 * @return 실제로 마이그레이션이 일어났으면 true (호출자가 저장 여부를 판단할 수 있게)
	 */
	bool MigrateIfNeeded();

	// 재화 태그(MetaProgression.Currency.*)별 보유량. 항목이 없는 재화는 0으로 취급한다.
	UPROPERTY()
	TMap<FGameplayTag, int32> CurrencyAmounts;

	// 첫 클리어를 완료한 보스의 BossID 태그 모음 — 반복 클리어 보상 판정에 사용
	UPROPERTY()
	FGameplayTagContainer ClearedBossTags;

	// 이 슬롯이 영구 보유한 인벤토리 항목. 런 전용 상태(카드/임시 버프/미정산 재화)는 여기에 들어가지 않는다
	UPROPERTY()
	TArray<FACSavedInventoryEntry> InventoryEntries;

	// 마지막으로 장착한 아이템의 정의 ID. 무효하면 장착 이력이 없다는 뜻이다
	UPROPERTY()
	FPrimaryAssetId EquippedItemDefinitionId;

	/**
	 * 세이브 데이터 구조 변경 시 마이그레이션 분기에 사용할 버전.
	 * 기본값을 1로 유지해야 한다 — UE의 태그드 프로퍼티 직렬화는 CDO 기본값과 같은 값을 파일에 쓰지 않으므로,
	 * 버전 1 파일에는 SaveVersion이 아예 기록되어 있지 않다. 기본값을 2로 올리면 옛 파일이 최신 버전으로 오인된다.
	 */
	UPROPERTY()
	int32 SaveVersion = 1;
};
