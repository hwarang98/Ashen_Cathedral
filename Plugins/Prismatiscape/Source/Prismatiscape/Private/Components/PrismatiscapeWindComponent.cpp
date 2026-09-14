// Copyright 2024, PrismaticaDev. All rights reserved.

#include "Components/PrismatiscapeWindComponent.h"

#include "Manager/PrismatiscapeManager.h"

UPrismatiscapeWindComponent::UPrismatiscapeWindComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPrismatiscapeWindComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPrismatiscapeWindComponent::GetWindCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength)
{
	if(bSkipBlueprintGatherWindCapsules) return;
	
	if(GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		TArray<FVector4> BP_StartLocationAndRadius;
		TArray<FVector4> BP_EndLocationAndRadius;
		TArray<FVector4> BP_StartVelocityAndStrength;
		TArray<FVector4> BP_EndVelocityAndStrength;

		BP_GetWindCapsules(BP_StartLocationAndRadius, BP_EndLocationAndRadius, BP_StartVelocityAndStrength, BP_EndVelocityAndStrength);

		StartLocationAndRadius.Append(BP_StartLocationAndRadius);
		EndLocationAndRadius.Append(BP_EndLocationAndRadius);
		StartVelocityAndStrength.Append(BP_StartVelocityAndStrength);
		EndVelocityAndStrength.Append(BP_EndVelocityAndStrength);
	}
}

void UPrismatiscapeWindComponent::ToggleDrawing_Implementation(bool bEnabled)
{
	Super::ToggleDrawing_Implementation(bEnabled);

	if(bEnabled)
	{
		APrismatiscapeManager::Get(this)->WindComponents.Add(this);
	}
	else
	{
		APrismatiscapeManager::Get(this)->WindComponents.RemoveSingleSwap(this);
	}
}
