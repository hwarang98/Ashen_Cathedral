// 어빌리티마다 서로 다른 쿨다운 시간을 런타임에 주입받는 공용 쿨다운 GameplayEffect — 어빌리티별로 GE 에셋을 따로 만들 필요가 없다

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ACGameplayEffect_DynamicCooldown.generated.h"

/**
 * 다이나믹 쿨다운 전용 GameplayEffect.
 *
 * Duration을 SetByCaller(Shared.SetByCaller.CooldownDuration)로 받아, 적용 시점에 어빌리티가 주입한 값으로 지속시간이 결정된다.
 * 쿨다운을 식별하는 태그는 어빌리티의 ApplyCooldown에서 DynamicGrantedTags로 주입하므로, 이 GE는 태그를 정적으로 갖지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayEffect_DynamicCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UACGameplayEffect_DynamicCooldown();
};
