// Run 중 제공할 보상 카드 전체 목록을 보관하는 DataAsset

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Structs/ACStructTypes.h"
#include "ACDataAsset_RewardCardList.generated.h"

/**
 * @brief 보상 카드 목록 DataAsset.
 *
 * 레벨/GameMode에서 UACRewardCardComponent에 할당해 사용한다.
 * 각 FACRewardCardData 항목에 카드 ID, 희귀도, GE 클래스 등을 설정한다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACDataAsset_RewardCardList : public UDataAsset
{
	GENERATED_BODY()

public:
	// 이 DataAsset에 등록된 모든 보상 카드 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCards")
	TArray<FACRewardCardData> Cards;
};
