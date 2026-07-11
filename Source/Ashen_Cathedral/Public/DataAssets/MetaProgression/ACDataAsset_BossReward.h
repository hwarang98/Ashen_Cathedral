// 보스 1체의 성흔 조각 보상 정보를 담는 DataAsset — 보스 캐릭터 BP가 자신의 BossRewardData로 직접 보유한다

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ACDataAsset_BossReward.generated.h"

/**
 * @brief 보스별 성흔 조각 보상 정의 DataAsset.
 *
 * 보스 캐릭터 BP(AACEnemyCharacter 파생)의 BossRewardData 프로퍼티에 할당해 사용한다.
 * BossID는 UACSaveGame_MetaProgression::ClearedBossTags의 첫 클리어 판정 키로 쓰인다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACDataAsset_BossReward : public UDataAsset
{
	GENERATED_BODY()

public:
	// 이 보스를 식별하는 태그. SaveGame의 첫 클리어 여부 판정 키로 사용된다 (예: MetaProgression.BossID.AshenKnight)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MetaProgression", meta = (Categories = "MetaProgression.BossID"))
	FGameplayTag BossID;

	// 이 보스를 처음 처치했을 때 지급할 성흔 조각 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MetaProgression", meta = (ClampMin = 0))
	int32 FirstClearScarFragments = 0;

	// 이 보스를 반복 처치했을 때 지급할 성흔 조각 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MetaProgression", meta = (ClampMin = 0))
	int32 RepeatClearScarFragments = 0;
};
