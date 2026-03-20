// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Player/ACPlayerAnimInstance.h"
#include "ACGameplayTags.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UACPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningPlayerCharacter = Cast<AACPlayerCharacter>(OwningCharacter);
}

void UACPlayerAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!OwningPlayerCharacter || !OwningMovementComponent)
	{
		return;
	}

	IsCrouching = OwningMovementComponent->IsCrouching();
	IsSprinting = DoesOwnerHaveTag(ACGameplayTags::Shared_Status_Sprinting);

	// 착지하면 bJumpTriggered 리셋
	if (!IsFalling)
	{
		bJumpTriggered = false;
	}

	// 이동 방향 계산
	if (GroundSpeed > KINDA_SMALL_NUMBER)
	{
		MoveDirection = UKismetAnimationLibrary::CalculateDirection(OwningCharacter->GetVelocity(), OwningCharacter->GetActorRotation());
	}
	else
	{
		MoveDirection = 0.f; // 멈춰있으면 방향 없음
	}
}

void UACPlayerAnimInstance::OnOwnerJumped()
{
	bJumpTriggered = true;

	const FVector Vel = OwningCharacter->GetVelocity();
	JumpDirection = Vel.SizeSquared2D() > KINDA_SMALL_NUMBER ? UKismetAnimationLibrary::CalculateDirection(Vel, OwningCharacter->GetActorRotation()) : 0.f;
}