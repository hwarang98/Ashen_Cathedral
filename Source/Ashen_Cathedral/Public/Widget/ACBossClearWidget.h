// 보스 클리어 후 표시되는 클리어 연출 위젯 C++ 베이스

#pragma once

#include "CoreMinimal.h"
#include "Widget/ACWidgetBase.h"
#include "ACBossClearWidget.generated.h"

/**
 * @brief Boss Clear 연출 위젯 C++ 베이스.
 *
 * Blueprint(WBP_BossClear)에서 이 클래스를 상속하여 UI를 구성한다.
 * 진행 버튼 없이 연출만 담당하며, UPlayerUIComponent가 PlayClearSequence로 연출을 재생시키고
 * 일정 시간 뒤 위젯을 직접 제거한다. 실제 다음 스테이지 진행은 AACStageExitPoint 상호작용이 담당한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACBossClearWidget : public UACWidgetBase
{
	GENERATED_BODY()

protected:
	// C++에서 클리어 연출(밴드 페이드인 → 홀드 → 페이드아웃)을 재생시킬 때 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "BossClear")
	void BP_PlayClearSequence(const FText& InClearText);

public:
	FORCEINLINE void PlayClearSequence(const FText& InText) { BP_PlayClearSequence(InText); }
};
