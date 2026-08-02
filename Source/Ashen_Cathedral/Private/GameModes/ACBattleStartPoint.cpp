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
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACBattleStartPoint] AACGameMode를 찾지 못해 전투 시작을 처리할 수 없습니다. 레벨의 GameMode Override를 확인하세요."));
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
