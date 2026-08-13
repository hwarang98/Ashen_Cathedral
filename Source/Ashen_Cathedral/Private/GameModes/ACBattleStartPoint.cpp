// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ACBattleStartPoint.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "DataAssets/Run/ACDataAsset_RunDefinition.h"
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
	// 구성이 없으면 레벨 이동을 시도조차 하지 않는다 — 잘못된 목적지로 넘어가면 되돌릴 방법이 없다
	if (!RunDefinition || RunDefinition->OrderedStages.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACBattleStartPoint] RunDefinition이 비어 있어 전투 시작을 거부했습니다. 이 액터의 Run > Run Definition을 설정하세요."));
		return;
	}

	UWorld* World = GetWorld();
	AACGameMode* ACGameMode = World ? World->GetAuthGameMode<AACGameMode>() : nullptr;
	if (!ACGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACBattleStartPoint] AACGameMode를 찾지 못해 전투 시작을 처리할 수 없습니다. 레벨의 GameMode Override를 확인하세요."));
		return;
	}

	ACGameMode->RequestStartRun(RunDefinition);
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
