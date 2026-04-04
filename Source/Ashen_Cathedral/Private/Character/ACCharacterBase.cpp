// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ACCharacterBase.h"
#include "Components/Input/ACInputComponent.h"
#include "DataAssets/Startup/ACDataAsset_StartupDataBase.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "MotionWarpingComponent.h"

AACCharacterBase::AACCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ACAbilitySystemComponent = CreateDefaultSubobject<UACAbilitySystemComponent>("Ability System Component");
	ACAttributeSet = CreateDefaultSubobject<UACAttributeSet>("Attribute Set");
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>("MotionWarpingComponent");
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

	if (ACAbilitySystemComponent)
	{
		ACAbilitySystemComponent->InitAbilityActorInfo(this, this);

		ensureMsgf(!CharacterStartUpData.IsNull(), TEXT("Forgot to assign start up data to %s"), *GetName());
	}
}

UAbilitySystemComponent* AACCharacterBase::GetAbilitySystemComponent() const
{
	return GetACAbilitySystemComponent();
}

UPawnCombatComponent* AACCharacterBase::GetPawnCombatComponent() const
{
	return nullptr;
}