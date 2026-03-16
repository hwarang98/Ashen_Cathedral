// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ACCharacterBase.h"

#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"

AACCharacterBase::AACCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ACAbilitySystemComponent = CreateDefaultSubobject<UACAbilitySystemComponent>("Ability System Component");
	ACAttributeSet = CreateDefaultSubobject<UACAttributeSet>("Attribute Set");
}

void AACCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}


void AACCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AACCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

UAbilitySystemComponent* AACCharacterBase::GetAbilitySystemComponent() const
{
	return nullptr;
}
