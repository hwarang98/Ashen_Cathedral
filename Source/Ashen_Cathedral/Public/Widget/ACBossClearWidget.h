// 보스 클리어 후 표시되는 진행(Next Boss / Return To Lobby) 위젯 C++ 베이스

#pragma once

#include "CoreMinimal.h"
#include "Widget/ACWidgetBase.h"
#include "ACBossClearWidget.generated.h"

// 플레이어가 진행 버튼(Next Boss / Return To Lobby)을 클릭했을 때 C++로 콜백을 전달하는 델리게이트
DECLARE_DELEGATE(FOnNextRequested);

/**
 * @brief Boss Clear UI 위젯 C++ 베이스.
 *
 * Blueprint(WBP_BossClear)에서 이 클래스를 상속하여 UI를 구성한다.
 * BP의 진행 버튼 OnClicked에서 NotifyNextRequested를 호출하면
 * C++의 UPlayerUIComponent로 결과가 전달된다.
 * 버튼의 활성 상태/문구는 BP_SetNextButtonEnabled / BP_SetNextButtonText로 C++에서 갱신한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACBossClearWidget : public UACWidgetBase
{
	GENERATED_BODY()

public:
	// UPlayerUIComponent에서 바인딩: 진행 버튼 클릭 시 호출
	FOnNextRequested OnNextRequestedDelegate;

	/**
	 * @brief Blueprint에서 호출: 플레이어가 진행 버튼을 클릭했을 때 이 함수를 호출
	 */
	UFUNCTION(BlueprintCallable, Category = "BossClear")
	void NotifyNextRequested();

protected:
	// C++에서 능력 선택 완료 여부에 따라 진행 버튼의 활성 상태를 갱신할 때 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "BossClear")
	void BP_SetNextButtonEnabled(bool bEnabled);

	// C++에서 일반 보스(Next Boss) / 최종 보스(Return To Lobby) 문구를 설정할 때 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "BossClear")
	void BP_SetNextButtonText(const FText& InText);

public:
	FORCEINLINE void SetNextButtonEnabled(bool bEnabled) { BP_SetNextButtonEnabled(bEnabled); }
	FORCEINLINE void SetNextButtonText(const FText& InText) { BP_SetNextButtonText(InText); }
};
