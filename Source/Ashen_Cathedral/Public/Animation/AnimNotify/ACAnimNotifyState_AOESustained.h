// 몽타주의 지속형 AOE 구간(대쉬/채널링 등)에 배치하면 NotifyBegin/End에서 자동으로 Shared.Event.AOE.Sustained.Start/End를 발송하는 AnimNotifyState.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ACAnimNotifyState_AOESustained.generated.h"

/**
 * @brief 지속형 AOE 판정 구간을 표시하는 AnimNotifyState.
 *
 * 대쉬 돌진, 채널링, 회전 베기 등 판정이 일정 시간 이어지는 몽타주 구간에 하나만 배치하면
 * NotifyBegin에서 Shared.Event.AOE.Sustained.Start를, NotifyEnd에서 Shared.Event.AOE.Sustained.End를
 * 소유 액터에게 자동으로 발송한다.
 * 두 이벤트 태그를 직접 설정할 필요가 없으며, 몽타주가 중단되어도 엔진이 NotifyEnd를 호출하므로
 * 판정 구간이 항상 짝을 맞춰 종료된다.
 * 데미지는 직접 적용하지 않으며, 이벤트를 수신한 GameplayAbility가 판정/적용을 담당한다.
 */
UCLASS(meta = (DisplayName = "ANS_AOESustained"))
class ASHEN_CATHEDRAL_API UACAnimNotifyState_AOESustained : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
