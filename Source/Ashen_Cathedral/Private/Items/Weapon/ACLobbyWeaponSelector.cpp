// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/ACLobbyWeaponSelector.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "Components/SphereComponent.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Items/Weapon/ACWeapon.h"
#include "Items/Weapon/ACWeaponBase.h"
#include "Subsystems/ACWeaponSelectionSubsystem.h"
#include "TimerManager.h"

AACLobbyWeaponSelector::AACLobbyWeaponSelector()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Sphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnInteractionSphereEndOverlap);

	PreviewAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("Preview Anchor"));
	PreviewAnchor->SetupAttachment(InteractionSphere);
}

void AACLobbyWeaponSelector::BeginPlay()
{
	Super::BeginPlay();

	if (UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem())
	{
		Subsystem->OnSelectedWeaponChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandleSelectedWeaponChanged);

		BP_OnSelectionStateChanged(WeaponData != nullptr && Subsystem->GetSelectedWeaponData() == WeaponData);
	}

	SpawnPreviewWeapon();
}

void AACLobbyWeaponSelector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem())
	{
		Subsystem->OnSelectedWeaponChangedDelegate.RemoveDynamic(this, &ThisClass::HandleSelectedWeaponChanged);
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	if (PreviewWeapon)
	{
		PreviewWeapon->Destroy();
		PreviewWeapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AACLobbyWeaponSelector::Interact(APawn* InstigatorPawn)
{
	if (bInteractionPending)
	{
		return;
	}

	if (!WeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACLobbyWeaponSelector] %s에 WeaponData가 지정되지 않았습니다."), *GetName());
		return;
	}

	UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem();
	if (!Subsystem)
	{
		return;
	}

	if (!Subsystem->CanChangeWeaponSelection())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACLobbyWeaponSelector] 다른 무기 교체가 진행 중이라 요청을 거부했습니다."));
		return;
	}

	if (Subsystem->GetSelectedWeaponData() == WeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACLobbyWeaponSelector] 이미 %s를 선택한 상태라 교체하지 않습니다."), *WeaponData->GetName());
		return;
	}

	AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(InstigatorPawn);
	UPlayerCombatComponent* CombatComponent = PlayerCharacter ? PlayerCharacter->GetPawnCombatComponent() : nullptr;

	if (!CombatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACLobbyWeaponSelector] 상호작용한 대상이 플레이어가 아니거나 CombatComponent가 없습니다."));
		return;
	}

	if (CombatComponent->IsWeaponSwapInProgress())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACLobbyWeaponSelector] 무기 교체가 아직 진행 중입니다."));
		return;
	}

	bInteractionPending = true;
	bSwapCommitted = false;
	PendingInstigator = PlayerCharacter;

	// 연출 구간에도 다른 선택대 상호작용과 전투 시작을 막아야 한다
	Subsystem->SetWeaponChangeInProgress(true);

	BP_OnFlyToPlayerStarted(PlayerCharacter);

	// 연출 시간이 0이면 기다리지 않고 곧바로 교체한다
	if (FlyToPlayerDuration <= 0.f)
	{
		CommitWeaponSwap();
		return;
	}

	// 타임라인이 아니라 타이머가 드라이버다. BP 연출이 미구현이거나 잘못 배선돼도 교체는 반드시 커밋된다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FlyToPlayerTimerHandle, this, &ThisClass::CommitWeaponSwap, FlyToPlayerDuration, false);
	}
}

void AACLobbyWeaponSelector::CommitWeaponSwap()
{
	if (!bInteractionPending || bSwapCommitted)
	{
		return;
	}

	bSwapCommitted = true;

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlyToPlayerTimerHandle);
	}

	AACPlayerCharacter* PlayerCharacter = PendingInstigator.Get();
	UPlayerCombatComponent* CombatComponent = PlayerCharacter ? PlayerCharacter->GetPawnCombatComponent() : nullptr;

	if (!CombatComponent)
	{
		HandleWeaponSwapFinished(false);
		return;
	}

	// 양쪽 무기가 몽타주 없이 동기 완료될 수 있으므로 호출 전에 먼저 바인드한다
	CombatComponent->OnWeaponSwapFinishedDelegate.AddUniqueDynamic(this, &ThisClass::HandleWeaponSwapFinished);

	if (!CombatComponent->RequestWeaponSwap(WeaponData, bPlayUnequipMontage, bPlayEquipMontage))
	{
		HandleWeaponSwapFinished(false);
	}
}

void AACLobbyWeaponSelector::HandleWeaponSwapFinished(bool bSuccess)
{
	if (!bInteractionPending)
	{
		return;
	}

	if (const AACPlayerCharacter* PlayerCharacter = PendingInstigator.Get())
	{
		if (UPlayerCombatComponent* CombatComponent = PlayerCharacter->GetPawnCombatComponent())
		{
			CombatComponent->OnWeaponSwapFinishedDelegate.RemoveDynamic(this, &ThisClass::HandleWeaponSwapFinished);
		}
	}

	bInteractionPending = false;
	bSwapCommitted = false;
	PendingInstigator = nullptr;

	if (UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem())
	{
		Subsystem->SetWeaponChangeInProgress(false);
	}

	BP_OnSwapFinished(bSuccess);
}

void AACLobbyWeaponSelector::HandleSelectedWeaponChanged(UACDataAsset_WeaponData* OldWeaponData, UACDataAsset_WeaponData* NewWeaponData)
{
	BP_OnSelectionStateChanged(WeaponData != nullptr && NewWeaponData == WeaponData);
}

void AACLobbyWeaponSelector::SpawnPreviewWeapon()
{
	UWorld* World = GetWorld();

	if (!World || !WeaponData || !WeaponData->WeaponClassToSpawn)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PreviewWeapon = World->SpawnActor<AACWeaponBase>(WeaponData->WeaponClassToSpawn, PreviewAnchor->GetComponentTransform(), SpawnParams);
	if (!PreviewWeapon)
	{
		return;
	}

	PreviewWeapon->AttachToComponent(PreviewAnchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// 표시 전용이므로 무기 콜리전이 데미지 경로를 타지 않게 완전히 끈다
	PreviewWeapon->SetActorEnableCollision(false);
	PreviewWeapon->SetActorTickEnabled(false);

	// bHideUntilEquipped 무기라도 로비에서는 보여야 한다
	PreviewWeapon->ShowWeapon();
}

UACWeaponSelectionSubsystem* AACLobbyWeaponSelector::GetWeaponSelectionSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr;
}

void AACLobbyWeaponSelector::OnInteractionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
	)
{
	if (AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OtherActor))
	{
		PlayerCharacter->SetCurrentInteractable(this);
	}
}

void AACLobbyWeaponSelector::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OtherActor))
	{
		PlayerCharacter->ClearCurrentInteractable(this);
	}
}

FText AACLobbyWeaponSelector::GetInteractionText() const
{
	return InteractionText;
}
