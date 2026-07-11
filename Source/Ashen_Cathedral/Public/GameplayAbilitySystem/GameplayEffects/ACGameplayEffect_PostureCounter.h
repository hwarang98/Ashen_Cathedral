// 패링 성공 시 공격자(Source) 본인에게 체간 역공 데미지를 적용하는 즉발형 GameplayEffect

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ACGameplayEffect_PostureCounter.generated.h"

/**
 * 패링 역공 전용 즉발형 GameplayEffect.
 *
 * SetByCaller(Shared.SetByCaller.PostureDamage) 값을 PostureDamageTaken 메타 어트리뷰트에 더해,
 * ACAttributeSet::HandlePostureDamage 파이프라인으로 체간 데미지를 누적시킨다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayEffect_PostureCounter : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UACGameplayEffect_PostureCounter();
};
