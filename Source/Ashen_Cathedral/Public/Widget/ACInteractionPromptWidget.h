// 상호작용 가능한 대상에 접근했을 때 표시되는 프롬프트 위젯 C++ 베이스

#pragma once

#include "CoreMinimal.h"
#include "Widget/ACWidgetBase.h"
#include "ACInteractionPromptWidget.generated.h"

/**
 * @brief 상호작용 프롬프트 UI 위젯 C++ 베이스.
 *
 * Blueprint(WBP_InteractionPrompt)에서 이 클래스를 상속하여 UI를 구성한다.
 * UPlayerUIComponent가 위젯을 한 번만 생성해 뷰포트에 올려두고 가시성만 토글하며,
 * 표시할 문구는 SetPromptText로 갱신한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACInteractionPromptWidget : public UACWidgetBase
{
	GENERATED_BODY()

protected:
	// C++에서 상호작용 대상의 문구를 갱신할 때 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BP_SetPromptText(const FText& InText);

public:
	FORCEINLINE void SetPromptText(const FText& InText) { BP_SetPromptText(InText); }
};
