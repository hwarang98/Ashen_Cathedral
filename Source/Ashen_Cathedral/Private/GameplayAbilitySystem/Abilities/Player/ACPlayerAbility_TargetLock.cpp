// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_TargetLock.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ScalableFloat.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/SizeBox.h"
#include "Controllers/ACPlayerController.h"
#include "Widget/ACWidgetBase.h"
#include "Blueprint/WidgetTree.h"

UACPlayerAbility_TargetLock::UACPlayerAbility_TargetLock()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_TargetLock);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_TargetLock);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void UACPlayerAbility_TargetLock::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	TryLockOnTarget();
	InitTargetLockMappingContext();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UACPlayerAbility_TargetLock::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CleanUp();
	ResetTargetLockMappingContext();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACPlayerAbility_TargetLock::OnTargetLockTick(float DeltaTime)
{
	if (!CurrentLockedActor ||
		UACFunctionLibrary::NativeDoesActorHaveTag(CurrentLockedActor, ACGameplayTags::Shared_Status_Dead) ||
		UACFunctionLibrary::NativeDoesActorHaveTag(GetPlayerCharacterFromActorInfo(), ACGameplayTags::Shared_Status_Dead)
	)
	{
		CancelTargetLockAbility();
		return;
	}

	SetTargetLockWidgetPosition();

	AACPlayerController* PlayerController = GetPlayerControllerFromActorInfo();
	AACPlayerCharacter* PlayerCharacter = GetPlayerCharacterFromActorInfo();

	if (!PlayerController || !PlayerCharacter)
	{
		return;
	}

	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerCharacter->GetActorLocation(), CurrentLockedActor->GetActorLocation());
	LookAtRotation -= FRotator(TargetLockCameraOffsetDistance, 0.f, 0.f);

	const FRotator CurrentControlRotation = PlayerController->GetControlRotation();
	const FRotator TargetRotation = FMath::RInterpTo(CurrentControlRotation, LookAtRotation, DeltaTime, TargetLockRotationInterpSpeed);

	PlayerController->SetControlRotation(FRotator(TargetRotation.Pitch, TargetRotation.Yaw, 0.f));

	const bool bIsRolling = UACFunctionLibrary::NativeDoesActorHaveTag(PlayerCharacter, ACGameplayTags::Player_Status_Rolling);
	if (!bIsRolling)
	{
		PlayerCharacter->SetActorRotation(FRotator(0.f, TargetRotation.Yaw, 0.f));
	}
}

void UACPlayerAbility_TargetLock::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
	GetAvailableActorsToLock();

	TArray<AActor*> ActorsOnLeft;
	TArray<AActor*> ActorsOnRight;
	AActor* NewTargetToLock = nullptr;

	GetAvailableActorsAroundTarget(ActorsOnLeft, ActorsOnRight);

	if (InSwitchDirectionTag == ACGameplayTags::Player_Event_SwitchTarget_Left)
	{
		NewTargetToLock = GetNearestTargetFromAvailableActors(ActorsOnLeft);
	}
	else
	{
		NewTargetToLock = GetNearestTargetFromAvailableActors(ActorsOnRight);
	}

	if (NewTargetToLock)
	{
		CurrentLockedActor = NewTargetToLock;
	}
}

void UACPlayerAbility_TargetLock::TryLockOnTarget()
{
	GetAvailableActorsToLock();

	if (AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}

	CurrentLockedActor = GetNearestTargetFromAvailableActors(AvailableActorsToLock);

	if (CurrentLockedActor)
	{
		GetPlayerCharacterFromActorInfo()->GetCharacterMovement()->bOrientRotationToMovement = false;
		DrawTargetLockWidget();
		InitTargetLockMovement();
	}
	else
	{
		CancelTargetLockAbility();
	}
}

void UACPlayerAbility_TargetLock::GetAvailableActorsToLock()
{
	AvailableActorsToLock.Empty();
	TArray<FHitResult> BoxTraceHits;

	UKismetSystemLibrary::BoxTraceMultiForObjects(
		GetPlayerCharacterFromActorInfo(),
		GetPlayerCharacterFromActorInfo()->GetActorLocation(),
		GetPlayerCharacterFromActorInfo()->GetActorLocation() + GetPlayerCharacterFromActorInfo()->GetActorForwardVector() * BoxTraceDistance,
		TraceBoxSize / 2.f,
		GetPlayerCharacterFromActorInfo()->GetActorForwardVector().ToOrientationRotator(),
		BoxTraceChannel,
		false,
		TArray<AActor*>(),
		bShowPersistentDebugShape ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,
		BoxTraceHits,
		true
		);

	for (const FHitResult& TraceHit : BoxTraceHits)
	{
		if (AActor* HitActor = TraceHit.GetActor())
		{
			if (HitActor != GetPlayerCharacterFromActorInfo())
			{
				AvailableActorsToLock.AddUnique(HitActor);
			}
		}
	}
}

AActor* UACPlayerAbility_TargetLock::GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
	float ClosestDistance = 0.f;
	return UGameplayStatics::FindNearestActor(GetPlayerCharacterFromActorInfo()->GetActorLocation(), InAvailableActors, ClosestDistance);
}

void UACPlayerAbility_TargetLock::GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight)
{

	if (!CurrentLockedActor || AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}

	const FVector PlayerLocation = GetPlayerCharacterFromActorInfo()->GetActorLocation();
	const FVector PlayerToCurrentNormalized = (CurrentLockedActor->GetActorLocation() - PlayerLocation).GetSafeNormal();

	for (AActor* AvailableActor : AvailableActorsToLock)
	{
		if (!AvailableActor || AvailableActor == CurrentLockedActor)
		{
			continue;
		}

		const FVector PlayerToAvailableNormalized = (AvailableActor->GetActorLocation() - PlayerLocation).GetSafeNormal();

		const FVector CrossResult = FVector::CrossProduct(PlayerToCurrentNormalized, PlayerToAvailableNormalized);

		if (CrossResult.Z > 0.f)
		{
			OutActorsOnRight.AddUnique(AvailableActor);
		}
		else
		{
			OutActorsOnLeft.AddUnique(AvailableActor);
		}
	}
}

void UACPlayerAbility_TargetLock::DrawTargetLockWidget()
{
	if (!DrawnTargetLockWidget)
	{
		checkf(TargetLockWidgetClass, TEXT("블루프린트에 유효한 위젯 클래스를 할당하는 것을 잊었습니다."));

		DrawnTargetLockWidget = CreateWidget<UACWidgetBase>(GetPlayerControllerFromActorInfo(), TargetLockWidgetClass);

		DrawnTargetLockWidget->AddToViewport();
	}
}

void UACPlayerAbility_TargetLock::SetTargetLockWidgetPosition()
{
	if (!DrawnTargetLockWidget || !CurrentLockedActor)
	{
		CancelTargetLockAbility();
		return;
	}

	FVector2D ScreenPosition;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		GetPlayerControllerFromActorInfo(),
		CurrentLockedActor->GetActorLocation(),
		ScreenPosition,
		true
		);

	if (TargetLockWidgetSize == FVector2D::ZeroVector)
	{
		DrawnTargetLockWidget->WidgetTree->ForEachWidget(
			[this](UWidget* FoundWidget) {
				if (const USizeBox* FoundSizeBox = Cast<USizeBox>(FoundWidget))
				{
					TargetLockWidgetSize.X = FoundSizeBox->GetWidthOverride();
					TargetLockWidgetSize.Y = FoundSizeBox->GetHeightOverride();
				}
			}
			);
	}

	ScreenPosition -= (TargetLockWidgetSize / 2.f);
	ScreenPosition.Y -= TargetLockWidgetVerticalOffset;

	DrawnTargetLockWidget->SetPositionInViewport(ScreenPosition, false);
}

void UACPlayerAbility_TargetLock::InitTargetLockMovement()
{
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	CachedDefaultMaxWalkSpeed = GetPlayerCharacterFromActorInfo()->GetACAttributeSet()->GetMoveSpeed();

	UGameplayEffect* DynamicGameplayEffect = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_TargetLock_MoveSpeed"));
	DynamicGameplayEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo& ModInfo = DynamicGameplayEffect->Modifiers.AddDefaulted_GetRef();
	ModInfo.Attribute = UACAttributeSet::GetMoveSpeedAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Override;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(TargetLockMaxWalkSpeed));

	TargetLockSpeedEffectHandle = ASC->ApplyGameplayEffectToSelf(DynamicGameplayEffect, 1.f, ASC->MakeEffectContext());
}

void UACPlayerAbility_TargetLock::InitTargetLockMappingContext()
{
	const ULocalPlayer* LocalPlayer = GetPlayerControllerFromActorInfo()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem)

	Subsystem->AddMappingContext(TargetLockMappingContext, 3);
}

void UACPlayerAbility_TargetLock::CancelTargetLockAbility()
{
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UACPlayerAbility_TargetLock::CleanUp()
{
	AvailableActorsToLock.Empty();

	CurrentLockedActor = nullptr;

	GetPlayerCharacterFromActorInfo()->GetCharacterMovement()->bOrientRotationToMovement = true;
	ResetTargetLockMovement();

	if (DrawnTargetLockWidget)
	{
		DrawnTargetLockWidget->RemoveFromParent();
	}

	DrawnTargetLockWidget = nullptr;
	TargetLockWidgetSize = FVector2D::ZeroVector;

}

void UACPlayerAbility_TargetLock::ResetTargetLockMovement()
{
	if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveGameplayEffect(TargetLockSpeedEffectHandle);
	}
	TargetLockSpeedEffectHandle.Invalidate();
}

void UACPlayerAbility_TargetLock::ResetTargetLockMappingContext()
{
	if (!GetPlayerControllerFromActorInfo())
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = GetPlayerControllerFromActorInfo()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem)

	Subsystem->RemoveMappingContext(TargetLockMappingContext);
}