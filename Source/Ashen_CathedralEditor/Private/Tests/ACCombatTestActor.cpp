#include "Tests/ACCombatTestActor.h"

#include "Components/UI/PawnUIComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

AACCombatTestActor::AACCombatTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UACAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	PawnUIComponent = CreateDefaultSubobject<UPawnUIComponent>(TEXT("PawnUIComponent"));
}

UAbilitySystemComponent* AACCombatTestActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UPawnUIComponent* AACCombatTestActor::GetPawnUIComponent() const
{
	return PawnUIComponent;
}
