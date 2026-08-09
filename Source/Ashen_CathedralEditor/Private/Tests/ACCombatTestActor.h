// 전투 수식 자동화 테스트 전용 최소 액터 — UACAttributeSet이 요구하는 ASC와 PawnUI 컴포넌트만 갖춘다

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PawnUIInterface.h"
#include "ACCombatTestActor.generated.h"

class UACAbilitySystemComponent;
class UPawnUIComponent;

/**
 * @brief 임시 월드에 스폰해 쓰고 버리는 테스트용 액터.
 *
 * @note UACAttributeSet::PostGameplayEffectExecute가 IPawnUIInterface와 UPawnUIComponent를
 *       checkf로 강제하기 때문에, 평범한 AActor로는 GE를 적용하는 순간 크래시한다.
 *       게임에는 배치되지 않는다.
 */
UCLASS(NotBlueprintable, NotPlaceable, Hidden)
class AACCombatTestActor : public AActor, public IAbilitySystemInterface, public IPawnUIInterface
{
	GENERATED_BODY()

public:
	AACCombatTestActor();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UPawnUIComponent* GetPawnUIComponent() const override;

private:
	UPROPERTY()
	TObjectPtr<UACAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPawnUIComponent> PawnUIComponent;
};
