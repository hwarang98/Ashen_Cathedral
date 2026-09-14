// Copyright 2024, PrismaticaDev. All rights reserved.

#include "Components/PrismatiscapeInteractionBubbleComponent.h"

#include "Components/CapsuleComponent.h"
#include "Manager/PrismatiscapeManager.h"

UPrismatiscapeInteractionBubbleComponent::UPrismatiscapeInteractionBubbleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPrismatiscapeInteractionBubbleComponent::BeginPlay()
{
	Super::BeginPlay();

	if(!GetOwner()) return;

	UCapsuleComponent* Collision = GetOwner()->FindComponentByClass<UCapsuleComponent>();
	if(!Collision) return;

	HeightOffset = Collision->GetUnscaledCapsuleHalfHeight();
}

void UPrismatiscapeInteractionBubbleComponent::GetInteractionBubbles(TArray<FVector4>& LocationAndRadius, TArray<FVector4>& VelocityAndStrength)
{
	FVector4 LocRad = GetOwner()->GetActorLocation();
	LocRad.Z -= (HeightOffset * ActorScale);
	LocRad.W = Radius * ActorScale;
	
	LocationAndRadius.Add(LocRad);

	FVector4 VelStr = GetOwner()->GetVelocity();
	VelStr.W = 1.0f;

	VelocityAndStrength.Add(VelStr);

	if(bSkipBlueprintGatherInteractionBubbles) return;
	
	if(GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		TArray<FVector4> BP_LocationAndRadius;
		TArray<FVector4> BP_VelocityAndStrength;
		BP_GetInteractionBubbles(BP_LocationAndRadius, BP_VelocityAndStrength);

		LocationAndRadius.Append(BP_LocationAndRadius);
		VelocityAndStrength.Append(BP_VelocityAndStrength);
	}
}

void UPrismatiscapeInteractionBubbleComponent::ToggleDrawing_Implementation(const bool bEnabled)
{
	Super::ToggleDrawing_Implementation(bEnabled);

	if(bEnabled)
	{
		APrismatiscapeManager::Get(this)->InteractionBubbleComponents.Add(this);
	}
	else
	{
		APrismatiscapeManager::Get(this)->InteractionBubbleComponents.RemoveSingleSwap(this);
	}
}