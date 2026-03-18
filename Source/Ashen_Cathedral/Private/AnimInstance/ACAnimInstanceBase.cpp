// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/ACAnimInstanceBase.h"

#include "ACFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Character/ACCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

void UACAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningCharacter = Cast<AACCharacterBase>(TryGetPawnOwner());

	if (OwningCharacter)
	{
		OwningMovementComponent = OwningCharacter->GetCharacterMovement();
	}
}

void UACAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter || !OwningMovementComponent)
	{
		return;
	}

	GroundSpeed = OwningCharacter->GetVelocity().Size2D();

	IsFalling = OwningMovementComponent->IsFalling();

	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared() > KINDA_SMALL_NUMBER;
}

bool UACAnimInstanceBase::DoesOwnerHaveTag(const FGameplayTag TagToCheck) const
{
	if (APawn* OwningPawn = TryGetPawnOwner())
	{
		return UACFunctionLibrary::NativeDoesActorHaveTag(OwningPawn, TagToCheck);
	}

	return false;
}