// Copyright 2024, PrismaticaDev. All rights reserved.


#include "Components/PrismatiscapeDeformComponent.h"

#include "Manager/PrismatiscapeManager.h"


UPrismatiscapeDeformComponent::UPrismatiscapeDeformComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UPrismatiscapeDeformComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPrismatiscapeDeformComponent::ToggleDrawing_Implementation(bool bEnabled)
{
	Super::ToggleDrawing_Implementation(bEnabled);
	
	if(bEnabled)
	{
		APrismatiscapeManager::Get(this)->DeformComponents.Add(this);
	}
	else
	{
		APrismatiscapeManager::Get(this)->DeformComponents.RemoveSingleSwap(this);
	}
}

void UPrismatiscapeDeformComponent::GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength)
{
	if(bSkipBlueprintGatherDeformCapsules) return;

	if(GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		TArray<FVector4> BP_StartLocationAndRadius;
		TArray<FVector4> BP_EndLocationAndRadius;
		TArray<FVector4> BP_StartVelocityAndStrength;
		TArray<FVector4> BP_EndVelocityAndStrength;

		BP_GetDeformCapsules(BP_StartLocationAndRadius, BP_EndLocationAndRadius, BP_StartVelocityAndStrength, BP_EndVelocityAndStrength);

		StartLocationAndRadius.Append(BP_StartLocationAndRadius);
		EndLocationAndRadius.Append(BP_EndLocationAndRadius);
		StartVelocityAndStrength.Append(BP_StartVelocityAndStrength);
		EndVelocityAndStrength.Append(BP_EndVelocityAndStrength);
	}
}
