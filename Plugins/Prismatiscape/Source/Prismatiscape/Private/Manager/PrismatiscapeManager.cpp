// Copyright 2024, PrismaticaDev. All rights reserved.


#include "Manager/PrismatiscapeManager.h"

#include "DrawDebugHelpers.h"
#include "PrismatiscapeSettings.h"
#include "PrismatiscapeWorldSubsystem.h"
#include "Components/PrismatiscapeDeformComponent.h"
#include "Components/PrismatiscapeInteractionBubbleComponent.h"
#include "Components/PrismatiscapeWindComponent.h"


APrismatiscapeManager::APrismatiscapeManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

APrismatiscapeManager* APrismatiscapeManager::Get(const UObject* WorldContextObject)
{
	return UPrismatiscapeWorldSubsystem::GetPrismatiscapeManager(WorldContextObject);
}

void APrismatiscapeManager::BeginPlay()
{
	Super::BeginPlay();
}

void APrismatiscapeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetFollowLocationThisFrame();

	UpdateAllVisibilities();

	FlushAllArrays();

	GatherAllShapesFromRegisteredComponents();
	
	PostTick();

	DrawDebugShapes();
}

void DrawShapes(const UWorld* World, const TArray<FVector4>& StartLocationAndRadius, const TArray<FVector4>& EndLocationAndRadius, const TArray<FVector4>& StartVelocityAndStrength)
{
	for(int i = 0; i < StartLocationAndRadius.Num(); ++i)
	{
		FVector Start = StartLocationAndRadius[i];
		FVector End = EndLocationAndRadius[i];
		float Radius = StartLocationAndRadius[i].W;
		
		FVector4 ToColor = StartVelocityAndStrength[i] / 500.f;
		FLinearColor Color = FLinearColor(ToColor);

		Color += FLinearColor(1, 1, 1, 1);
		Color /= 2.f;
		
		DrawDebugCylinder(World, Start, End, Radius, 12, Color.ToFColor(true), false, 0, SDPG_World, 2);
	}
	
	// Draw debug sphere at world origin
	DrawDebugSphere(World, FVector::ZeroVector, 10, 12, FColor::Red, false, 0, SDPG_World, 2);
}

void APrismatiscapeManager::DrawDebugShapes_Implementation()
{
	UWorld* World = GetWorld();
	if(!World) return;
	
	if(UPrismatiscapeSettings::GetPrismatiscapeSettings()->GetDrawDebugShapes())
	{
		DrawShapes(World, CapsuleStartLocationAndRadius, CapsuleEndLocationAndRadius, CapsuleStartVelocityAndStrength);
		DrawShapes(World, WindCapsuleStartLocationAndRadius, WindCapsuleEndLocationAndRadius, WindCapsuleStartVelocityAndStrength);
	}
}

void APrismatiscapeManager::GatherAllShapesFromRegisteredComponents_Implementation()
{
	for (UPrismatiscapeDeformComponent* Component : DeformComponents)
	{
		if (!IsValid(Component)) continue;
		if (!Component->bIsVisible) continue;

		Component->GetDeformCapsules(CapsuleStartLocationAndRadius, CapsuleEndLocationAndRadius, CapsuleStartVelocityAndStrength, CapsuleEndVelocityAndStrength);
	}

	for (UPrismatiscapeWindComponent* Component : WindComponents)
	{
		if (!IsValid(Component)) continue;
		if (!Component->bIsVisible) continue;

		Component->GetWindCapsules(WindCapsuleStartLocationAndRadius, WindCapsuleEndLocationAndRadius, WindCapsuleStartVelocityAndStrength, WindCapsuleEndVelocityAndStrength);
	}

	for (UPrismatiscapeInteractionBubbleComponent* Component : InteractionBubbleComponents)
	{
		if (!IsValid(Component)) continue;
		if (!Component->bIsVisible) continue;

		Component->GetInteractionBubbles(InteractionBubbleLocationAndRadius, InteractionBubbleVelocityAndStrength);
	}
}

void APrismatiscapeManager::FlushAllArrays_Implementation()
{
	CapsuleStartLocationAndRadius.Reset();
	CapsuleEndLocationAndRadius.Reset();
	CapsuleStartVelocityAndStrength.Reset();
	CapsuleEndVelocityAndStrength.Reset();
	
	WindCapsuleStartLocationAndRadius.Reset();
	WindCapsuleEndLocationAndRadius.Reset();
	WindCapsuleStartVelocityAndStrength.Reset();
	WindCapsuleEndVelocityAndStrength.Reset();
	
	InteractionBubbleLocationAndRadius.Reset();
	InteractionBubbleVelocityAndStrength.Reset();
}

void APrismatiscapeManager::UpdateAllVisibilities_Implementation()
{
	// implemented in bp
}

void APrismatiscapeManager::SetFollowLocationThisFrame_Implementation()
{
	if(ComponentToFollow)
	{
		FollowLocationThisFrame = ComponentToFollow->GetComponentLocation();
	}
	else
	{
		FollowLocationThisFrame = FVector::ZeroVector;
	}
}
