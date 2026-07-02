// 공격 몽타주의 콤보 연계 구간을 표시하고 ASC에 ComboWindow 상태 태그를 부여하는 AnimNotifyState

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ACAnimNotifyState_ComboWindow.generated.h"

/**
 * @brief 몽타주에 배치해 콤보 연계 가능 구간(ComboWindow)을 정의하는 AnimNotifyState.
 *
 * NotifyBegin 시 Player.Status.ComboWindow 태그를 ASC에 추가하고,
 * NotifyEnd 시 제거한다. TriggerComboChain()이 이 태그를 확인해 체인 허용 여부를 결정한다.
 */
UCLASS(meta = (DisplayName = "Combo Window"))
class ASHEN_CATHEDRAL_API UACAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};