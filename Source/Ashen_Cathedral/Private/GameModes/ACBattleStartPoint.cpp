// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACBattleStartPoint.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "GameModes/ACGameMode.h"

AACBattleStartPoint::AACBattleStartPoint()
{
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Sphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnInteractionSphereEndOverlap);
}

void AACBattleStartPoint::Interact(APawn* InstigatorPawn)
{
	if (AACGameMode* ACGameMode = GetWorld()->GetAuthGameMode<AACGameMode>())
	{
		ACGameMode->RequestStartRun();
	}
}

FText AACBattleStartPoint::GetInteractionText() const
{
	return InteractionText;
}

void AACBattleStartPoint::OnInteractionSphereBeginOverlap(
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

void AACBattleStartPoint::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OtherActor))
	{
		PlayerCharacter->ClearCurrentInteractable(this);
	}
}
