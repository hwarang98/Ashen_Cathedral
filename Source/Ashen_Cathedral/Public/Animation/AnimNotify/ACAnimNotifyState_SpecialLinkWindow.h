// 피니셔 몽타주의 스페셜 연계 구간을 표시하고 ASC에 SpecialLinkWindow 상태 태그를 부여하는 AnimNotifyState

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ACAnimNotifyState_SpecialLinkWindow.generated.h"

/**
 * @brief 피니셔 몽타주 후반부에 배치해 스페셜 공격 연계 가능 구간(SpecialLinkWindow)을 정의하는 AnimNotifyState.
 *
 * NotifyBegin 시 Player.Status.SpecialLinkWindow 태그를 ASC에 추가하고, NotifyEnd 시 제거한다.
 * 일반 콤보 연계용인 ComboWindow와 역할이 분리되어 있으며,
 * UACPlayerAbility_Attack::TryTriggerSpecialAttack()이 피니셔 재생 중일 때 이 태그만 확인한다.
 */
UCLASS(meta = (DisplayName = "Special Link Window"))
class ASHEN_CATHEDRAL_API UACAnimNotifyState_SpecialLinkWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
