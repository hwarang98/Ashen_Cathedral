// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Player/ACPlayerCharacter.h"
#include "AnimInstance/Player/ACPlayerAnimInstance.h"
#include "Components/Input/ACInputComponent.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "ACGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ACGameplayTags.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

AACPlayerCharacter::AACPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// GameplayCameraComponent = CreateDefaultSubobject<UGameplayCameraComponent>("Gameplay Camera Component");
	// GameplayCameraComponent->SetupAttachment(GetRootComponent());

	PlayerCombatComponent = CreateDefaultSubobject<UPlayerCombatComponent>(TEXT("Player Combat Component"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 120.f);
	CameraBoom->bUsePawnControlRotation = true;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("View Camera"));
	ViewCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
}

void AACPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 생성자 시점엔 BP EditDefaultsOnly 값이 미적용 -> BeginPlay에서 초기화
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}


void AACPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!InputConfigDataAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[오류] InputConfigDataAsset이 비어있습니다! BP를 확인하세요."));
		return;
	}

	UACInputComponent* ACInputComponent = CastChecked<UACInputComponent>(PlayerInputComponent);

	if (const APlayerController* OwningPlayerController = GetController<APlayerController>())
	{
		UEnhancedInputLocalPlayerSubsystem* PlayerSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

		check(PlayerSubsystem);

		PlayerSubsystem->RemoveMappingContext(InputConfigDataAsset->DefaultMappingContext);
		PlayerSubsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext, 0);
	}

	ACInputComponent->BindNativeInputAction(InputConfigDataAsset, ACGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	ACInputComponent->BindNativeInputAction(InputConfigDataAsset, ACGameplayTags::InputTag_Move, ETriggerEvent::Completed, this, &ThisClass::StopSprint);
	ACInputComponent->BindNativeInputAction(InputConfigDataAsset, ACGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	ACInputComponent->BindNativeInputAction(InputConfigDataAsset, ACGameplayTags::InputTag_MouseLook, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	ACInputComponent->BindNativeInputAction(InputConfigDataAsset, ACGameplayTags::InputTag_Jump, ETriggerEvent::Triggered, this, &ThisClass::Jump);

	ACInputComponent->BindAbilityInputAction(InputConfigDataAsset, this, &ThisClass::Input_AbilityInputPressed, &ThisClass::Input_AbilityInputReleased);
}

void AACPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void AACPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if (UACPlayerAnimInstance* PlayerAnimInst = Cast<UACPlayerAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		PlayerAnimInst->OnOwnerJumped();
	}
}

UPlayerCombatComponent* AACPlayerCharacter::GetPawnCombatComponent() const
{
	return PlayerCombatComponent;
}

void AACPlayerCharacter::StopSprint()
{
	// 이동 입력 해제 시 Sprint 어빌리티를 태그로 취소
	if (ACAbilitySystemComponent)
	{
		FGameplayTagContainer SprintTag;
		SprintTag.AddTag(ACGameplayTags::Player_Ability_Sprint);
		ACAbilitySystemComponent->CancelAbilities(&SprintTag);
	}
}

void AACPlayerCharacter::Input_AbilityInputPressed(const FGameplayTag InInputTag)
{
	if (ACAbilitySystemComponent)
	{
		ACAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
	}
}

void AACPlayerCharacter::Input_AbilityInputReleased(const FGameplayTag InInputTag)
{
	if (ACAbilitySystemComponent)
	{
		ACAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
	}
}

void AACPlayerCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();
	const FRotator MovementRotation = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);

	if (MovementVector.Y != 0.f)
	{
		const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);

		AddMovementInput(ForwardDirection, MovementVector.Y);
	}

	if (MovementVector.X != 0.f)
	{
		const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AACPlayerCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	if (LookAxisVector.X != 0.f)
	{
		AddControllerYawInput(LookAxisVector.X);
	}
	if (LookAxisVector.Y != 0.f)
	{
		AddControllerPitchInput(LookAxisVector.Y);
	}

	// if (ACAbilitySystemComponent && ACAbilitySystemComponent->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_IsLockedOn))
	// {
	//
	// 	if (PlayerCombatComponent && FMath::Abs(LookAxisVector.X) > 0.5f)
	// 	{
	// 		PlayerCombatComponent->Server_SwitchLockOnTarget(LookAxisVector);
	// 	}
	// }
	// else
	// {
	//
	// 	if (LookAxisVector.X != 0.f)
	// 	{
	// 		AddControllerYawInput(LookAxisVector.X);
	// 	}
	// 	if (LookAxisVector.Y != 0.f)
	// 	{
	// 		AddControllerPitchInput(LookAxisVector.Y);
	// 	}
	// }
}