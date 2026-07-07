// 몽타주의 특정 프레임에 배치하면 자동으로 Shared.Event.AOE.Instant를 발송하는 단발형 AOE AnimNotify.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ACAnimNotify_AOEInstant.generated.h"

/**
 * @brief 단발형 AOE 판정 타이밍을 표시하는 AnimNotify.
 *
 * 몽타주의 타격 프레임에 하나만 배치하면 Notify 발생 시 Shared.Event.AOE.Instant를
 * 소유 액터에게 자동으로 발송한다. 태그를 직접 설정할 필요가 없다.
 * 데미지는 직접 적용하지 않으며, 이벤트를 수신한 GameplayAbility가 판정/적용을 담당한다.
 */
UCLASS(meta = (DisplayName = "AN_AOEInstant"))
class ASHEN_CATHEDRAL_API UACAnimNotify_AOEInstant : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
