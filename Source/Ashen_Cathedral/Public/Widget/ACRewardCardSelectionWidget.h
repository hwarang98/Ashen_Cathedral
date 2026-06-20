// 보스 클리어 후 카드 3장을 표시하고 플레이어 선택을 받는 위젯 C++ 베이스

#pragma once

#include "CoreMinimal.h"
#include "Widget/ACWidgetBase.h"
#include "Structs/ACStructTypes.h"
#include "ACRewardCardSelectionWidget.generated.h"

// 플레이어가 카드를 선택했을 때 C++로 콜백을 전달하는 델리게이트
DECLARE_DELEGATE_OneParam(FOnRewardCardSelected, FName /*CardID*/);

/**
 * @brief 보상 카드 선택 위젯 C++ 베이스.
 *
 * Blueprint(WBP_RewardCardSelection)에서 이 클래스를 상속하여 UI를 구성한다.
 * BP에서 BP_SetupCards를 구현하고, 카드 클릭 시 NotifyCardSelected를 호출하면
 * C++의 UACRewardCardComponent로 결과가 전달된다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACRewardCardSelectionWidget : public UACWidgetBase
{
	GENERATED_BODY()

public:
	// UACRewardCardComponent에서 바인딩: 카드 선택 완료 시 호출
	FOnRewardCardSelected OnCardSelectedDelegate;

	/**
	 * @brief UACRewardCardComponent가 위젯 생성 후 호출 — 표시할 카드 목록을 BP로 전달
	 * @param Cards 표시할 카드 표시 정보 배열 (카드 데이터 + 현재 중첩 수)
	 */
	void SetupCards(const TArray<FACRewardCardDisplayInfo>& Cards);

	/**
	 * @brief Blueprint에서 호출: 플레이어가 카드를 클릭했을 때 이 함수를 호출
	 * @param CardID 선택된 카드의 고유 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "RewardCard")
	void NotifyCardSelected(FName CardID);

protected:
	/**
	 * @brief Blueprint에서 구현: 카드 목록을 받아 슬롯 위젯을 생성하고 화면을 구성
	 * @param Cards 표시할 카드 정보 배열
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RewardCard")
	void BP_SetupCards(const TArray<FACRewardCardDisplayInfo>& Cards);
};
