// Copyright 2024, PrismaticaDev. All rights reserved.


#include "Components/PrismatiscapeDrawComponent.h"
#include "GameFramework/Actor.h"

UPrismatiscapeDrawComponent::UPrismatiscapeDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPrismatiscapeDrawComponent::BeginPlay()
{
	Super::BeginPlay();

	ToggleDrawing(bDrawByDefault);

	ActorScale = GetOwner()->GetActorScale().Z;
}

void UPrismatiscapeDrawComponent::ToggleDrawing_Implementation(bool bEnabled)
{
	
}