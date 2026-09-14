// Copyright 2024, PrismaticaDev. All rights reserved.

#include "Components/PrismatiscapeDeformComponentTwoSocket.h"
#include "TimerManager.h"
#include "Engine/World.h"

UPrismatiscapeDeformComponentTwoSocket::UPrismatiscapeDeformComponentTwoSocket()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPrismatiscapeDeformComponentTwoSocket::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UPrismatiscapeDeformComponentTwoSocket::SetMeshComponent, 0.2f, false);
}

void UPrismatiscapeDeformComponentTwoSocket::SetMeshComponent()
{
	MeshComponent = GetOwner()->FindComponentByClass<UMeshComponent>();
}

void UPrismatiscapeDeformComponentTwoSocket::CalculateVelocity_Implementation()
{
	CurrentStartVelocity = CurrentStartSocketLocation - MeshComponent->GetSocketLocation(SocketNameStart);
	CurrentEndVelocity = CurrentEndSocketLocation - MeshComponent->GetSocketLocation(SocketNameEnd);
}

void UPrismatiscapeDeformComponentTwoSocket::GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength)
{
	Super::GetDeformCapsules(StartLocationAndRadius, EndLocationAndRadius, StartVelocityAndStrength, EndVelocityAndStrength);

	TRACE_CPUPROFILER_EVENT_SCOPE(UPrismatiscapeDeformComponentTwoSocket::GetDeformCapsules);

	if(!MeshComponent) return;

	CalculateVelocity();

	CurrentStartSocketLocation = MeshComponent->GetSocketLocation(SocketNameStart);
	CurrentEndSocketLocation = MeshComponent->GetSocketLocation(SocketNameEnd);
	
	StartLocationAndRadius.Add(FVector4(CurrentStartSocketLocation, StartRadius));
	EndLocationAndRadius.Add(FVector4(CurrentEndSocketLocation, EndRadius));

	StartVelocityAndStrength.Add(FVector4(CurrentStartVelocity, StartStrength));
	EndVelocityAndStrength.Add(FVector4(CurrentEndVelocity, EndStrength));
}
