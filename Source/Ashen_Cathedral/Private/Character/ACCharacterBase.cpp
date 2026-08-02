// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ACCharacterBase.h"

#include "ACFunctionLibrary.h"
#include "Components/Input/ACInputComponent.h"
#include "DataAssets/Startup/ACDataAsset_StartupDataBase.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "MotionWarpingComponent.h"

#if AC_WEB_DEBUG
	#include "Debug/ACWebDebugSubsystem.h"
#endif

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

#if AC_WEB_DEBUG
		// 웹 디버그 타임라인의 태그 레인 — 상태 태그 구간을 구독한다(중복 등록은 내부에서 걸러진다)
		if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(this))
		{
			WebDebug->RegisterAbilitySystem(ACAbilitySystemComponent);
		}
#endif
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

UPawnUIComponent* AACCharacterBase::GetPawnUIComponent() const
{
	return nullptr;
}

void AACCharacterBase::OnAnimNotifyAddGameplayTags_Implementation(const FGameplayTagContainer& GameplayTags)
{
	UACFunctionLibrary::AddGameplayTagsToActor(this, GameplayTags);
}

void AACCharacterBase::OnAnimNotifyRemoveGameplayTags_Implementation(const FGameplayTagContainer& GameplayTags)
{
	UACFunctionLibrary::RemoveGameplayTagsFromActor(this, GameplayTags);
}