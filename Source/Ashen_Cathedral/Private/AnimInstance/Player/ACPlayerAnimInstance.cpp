// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Player/ACPlayerAnimInstance.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "KismetAnimationLibrary.h"

void UACPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningPlayerCharacter = Cast<AACPlayerCharacter>(OwningCharacter);
}

void UACPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningPlayerCharacter)
	{
		return;
	}

	if (UACAbilitySystemComponent* ASC = UACFunctionLibrary::NativeAbilitySystemComponentFromActor(OwningPlayerCharacter))
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		CurrentGameplayTags = OwnedTags.Filter(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Player.Weapon"))));
	}

	if (UPlayerCombatComponent* CombatComponent = OwningPlayerCharacter->GetPawnCombatComponent())
	{
		CurrentWeaponType = CombatComponent->GetPlayerCurrentWeaponType();
	}

	// 매 프레임 호출되므로 값이 실제로 바뀐 순간에만 출력한다
	if (bDebugLogWeaponType && (CurrentWeaponType != LastLoggedWeaponType || CurrentGameplayTags != LastLoggedWeaponTags))
	{
		LastLoggedWeaponType = CurrentWeaponType;
		LastLoggedWeaponTags = CurrentGameplayTags;

		UE_LOG(LogTemp, Warning, TEXT("[ACPlayerAnimInstance] CurrentWeaponType=%s, 무기 태그=%s"), *UEnum::GetValueAsString(CurrentWeaponType), *CurrentGameplayTags.ToStringSimple());
	}
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