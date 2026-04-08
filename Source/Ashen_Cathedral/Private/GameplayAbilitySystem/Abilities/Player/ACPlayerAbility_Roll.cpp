// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Roll.h"
#include "ACGameplayTags.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACPlayerAbility_Roll::UACPlayerAbility_Roll()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Roll);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_Rolling);

	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_Rolling);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACPlayerAbility_Roll::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AACPlayerCharacter* PlayerCharacter = GetACPlayerFromActorInfo();
	if (!PlayerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ERollDirection RollDirection;
	FVector WorldRollDirection;
	ResolveRollMovement(PlayerCharacter, RollDirection, WorldRollDirection);

	const UPlayerCombatComponent* CombatComp = PlayerCharacter->GetPawnCombatComponent();
	const bool bIsWeaponEquipped = CombatComp && CombatComp->GetPlayerCurrentEquippedWeapon() != nullptr;

	UAnimMontage* MontageToPlay = SelectMontage(RollDirection, bIsWeaponEquipped);
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float SafeDistance = CalculateSafeRollDistance(PlayerCharacter->GetActorLocation(), WorldRollDirection);
	const FVector TargetLocation = PlayerCharacter->GetActorLocation() + (WorldRollDirection * SafeDistance);

	SetupMotionWarping(TargetLocation);
	ApplyInvincibilityEffect();
	PlayRollMontage(MontageToPlay);
}

void UACPlayerAbility_Roll::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (InvincibilityEffectHandle.IsValid())
	{
		if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(InvincibilityEffectHandle);
		}
		InvincibilityEffectHandle.Invalidate();
	}

	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}
	MontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UACPlayerAbility_Roll::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UPlayerCombatComponent* CombatComp = GetPlayerCombatComponentFromActorInfo();
	const bool bIsWeaponEquipped = CombatComp && CombatComp->GetPlayerCurrentEquippedWeapon() != nullptr;

	return bIsWeaponEquipped ? !DodgeMontages.IsEmpty() : !RollMontages.IsEmpty();
}

void UACPlayerAbility_Roll::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACPlayerAbility_Roll::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UACPlayerAbility_Roll::ResolveRollMovement(const AACPlayerCharacter* PlayerCharacter, ERollDirection& OutDirection, FVector& OutWorldDir) const
{
	const FVector CachedInput = PlayerCharacter->GetLastMovementInputVector().GetSafeNormal();
	const FRotator YawRotation(0.f, PlayerCharacter->GetControlRotation().Yaw, 0.f);
	const FVector ForwardDir = YawRotation.RotateVector(FVector::ForwardVector);
	const FVector RightDir = YawRotation.RotateVector(FVector::RightVector);

	// 월드 입력을 카메라 기준 전방/우측으로 직접 투영 (UnrotateVector → 2D 왕복 대체)
	const FVector2D Input2D(FVector::DotProduct(CachedInput, ForwardDir), FVector::DotProduct(CachedInput, RightDir));

	// 입력이 없으면 카메라 전방으로 롤링
	const FVector2D SnappedInput = Input2D.IsNearlyZero() ? FVector2D(1.f, 0.f) : Input2D.GetSafeNormal();

	OutDirection = CalculateRollDirection(SnappedInput);
	OutWorldDir = (ForwardDir * SnappedInput.X + RightDir * SnappedInput.Y).GetSafeNormal();
}

UAnimMontage* UACPlayerAbility_Roll::SelectMontage(ERollDirection Direction, bool bIsWeaponEquipped) const
{
	const TMap<ERollDirection, TObjectPtr<UAnimMontage>>& Montages = bIsWeaponEquipped ? DodgeMontages : RollMontages;
	const TObjectPtr<UAnimMontage>* Found = Montages.Find(Direction);
	return (Found && *Found) ? Found->Get() : nullptr;
}

void UACPlayerAbility_Roll::ApplyInvincibilityEffect()
{
	if (!InvincibilityEffect)
	{
		return;
	}

	if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		InvincibilityEffectHandle = ASC->ApplyGameplayEffectToSelf(
			InvincibilityEffect->GetDefaultObject<UGameplayEffect>(),
			1.0f,
			ASC->MakeEffectContext()
			);
	}
}

void UACPlayerAbility_Roll::PlayRollMontage(UAnimMontage* Montage)
{
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.0f, NAME_None, false
		);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

ERollDirection UACPlayerAbility_Roll::CalculateRollDirection(const FVector2D& InputVector)
{
	float Angle = FMath::Atan2(InputVector.Y, InputVector.X) * (180.0f / PI);
	if (Angle < 0.0f)
	{
		Angle += 360.0f;
	}

	const int32 DirectionIndex = FMath::RoundToInt(Angle / 45.0f) % 8;

	switch (DirectionIndex)
	{
		case 0:
			return ERollDirection::Forward;

		case 1:
			return ERollDirection::ForwardRight;

		case 2:
			return ERollDirection::Right;

		case 3:
			return ERollDirection::BackwardRight;

		case 4:
			return ERollDirection::Backward;

		case 5:
			return ERollDirection::BackwardLeft;

		case 6:
			return ERollDirection::Left;

		case 7:
			return ERollDirection::ForwardLeft;

		default:
			return ERollDirection::Forward;
	}
}

float UACPlayerAbility_Roll::CalculateSafeRollDistance(const FVector& StartLocation, const FVector& Direction) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return DefaultRollDistance;
	}

	const AACPlayerCharacter* PlayerCharacter = GetACPlayerFromActorInfo();
	if (!PlayerCharacter)
	{
		return DefaultRollDistance;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerCharacter);
	QueryParams.bTraceComplex = false;

	FCollisionObjectQueryParams ObjectQueryParams;
	for (const auto& ObjectType : TraceObjectTypes)
	{
		ObjectQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjectType));
	}

	if (TraceObjectTypes.Num() == 0)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	}

	const float CapsuleHalfHeight = PlayerCharacter->GetCapsuleComponent()
		? PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 90.0f;

	// 수평 트레이스 - 장애물 감지
	const FVector HorizontalTraceStart = StartLocation + FVector(0.0f, 0.0f, CapsuleHalfHeight * 0.5f);
	const FVector HorizontalTraceEnd = HorizontalTraceStart + (Direction * DefaultRollDistance);

	FHitResult HorizontalHit;
	const bool bHitObstacle = World->LineTraceSingleByObjectType(
		HorizontalHit,
		HorizontalTraceStart,
		HorizontalTraceEnd,
		ObjectQueryParams,
		QueryParams
		);

	float SafeHorizontalDistance = DefaultRollDistance;
	if (bHitObstacle)
	{
		const float ObstacleDistance = (HorizontalHit.Location - HorizontalTraceStart).Size();
		SafeHorizontalDistance = FMath::Max(ObstacleDistance - 50.0f, 100.0f);

		if (bDebugDrawTrace)
		{
			DrawDebugLine(World, HorizontalTraceStart, HorizontalHit.Location, FColor::Green, false, 3.0f, 0, 3.0f);
			DrawDebugLine(World, HorizontalHit.Location, HorizontalTraceEnd, FColor::Red, false, 3.0f, 0, 3.0f);
			DrawDebugSphere(World, HorizontalHit.Location, 20.0f, 12, FColor::Red, false, 3.0f, 0, 3.0f);
		}
	}
	else if (bDebugDrawTrace)
	{
		DrawDebugLine(World, HorizontalTraceStart, HorizontalTraceEnd, FColor::Green, false, 3.0f, 0, 3.0f);
	}

	// 수직 트레이스 - 목표 위치의 절벽 감지
	const FVector TargetHorizontalLocation = StartLocation + (Direction * SafeHorizontalDistance);
	const FVector VerticalTraceStart = TargetHorizontalLocation + FVector(0.0f, 0.0f, 200.0f);
	const FVector VerticalTraceEnd = TargetHorizontalLocation - FVector(0.0f, 0.0f, 500.0f);

	FHitResult VerticalHit;
	const bool bHitGround = World->LineTraceSingleByObjectType(
		VerticalHit,
		VerticalTraceStart,
		VerticalTraceEnd,
		ObjectQueryParams,
		QueryParams
		);

	if (bDebugDrawTrace)
	{
		DrawDebugSphere(World, HorizontalTraceStart, 15.0f, 12, FColor::Blue, false, 3.0f, 0, 3.0f);
		DrawDebugSphere(World, TargetHorizontalLocation, 15.0f, 12, FColor::Yellow, false, 3.0f, 0, 3.0f);

		if (bHitGround)
		{
			DrawDebugLine(World, VerticalTraceStart, VerticalHit.Location, FColor::Cyan, false, 3.0f, 0, 3.0f);
			DrawDebugLine(World, VerticalHit.Location, VerticalTraceEnd, FColor::Magenta, false, 3.0f, 0, 3.0f);
			DrawDebugSphere(World, VerticalHit.Location, 20.0f, 12, FColor::Orange, false, 3.0f, 0, 3.0f);
		}
		else
		{
			DrawDebugLine(World, VerticalTraceStart, VerticalTraceEnd, FColor::Red, false, 3.0f, 0, 3.0f);
		}
	}

	if (!bHitGround)
	{
		return SafeHorizontalDistance * 0.5f;
	}

	const float FeetLocationZ = StartLocation.Z - CapsuleHalfHeight;
	const float HeightDifference = FeetLocationZ - VerticalHit.Location.Z;

	if (HeightDifference > 150.0f)
	{
		return 10.0f;
	}

	return SafeHorizontalDistance;
}

void UACPlayerAbility_Roll::SetupMotionWarping(const FVector& TargetLocation) const
{
	const AACPlayerCharacter* PlayerCharacter = GetACPlayerFromActorInfo();
	if (!PlayerCharacter)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarping = PlayerCharacter->GetMotionWarpingComponent();
	if (!MotionWarping)
	{
		return;
	}

	FRotator TargetRotation = (TargetLocation - PlayerCharacter->GetActorLocation()).Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
}