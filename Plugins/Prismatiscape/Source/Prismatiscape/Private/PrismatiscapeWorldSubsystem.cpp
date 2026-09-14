// Copyright 2024, PrismaticaDev. All rights reserved.


#include "PrismatiscapeWorldSubsystem.h"
#include "PrismatiscapeSettings.h"
#include "Manager/PrismatiscapeManager.h"
#include "Engine/World.h"

UPrismatiscapeWorldSubsystem* UPrismatiscapeWorldSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	return WorldContextObject->GetWorld()->GetSubsystem<UPrismatiscapeWorldSubsystem>();
}

APrismatiscapeManager* UPrismatiscapeWorldSubsystem::GetPrismatiscapeManager(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return UPrismatiscapeSettings::Get()->PrismatiscapeManager.Get();

	const UPrismatiscapeWorldSubsystem* ThisSubsystem = Get(WorldContextObject);
	return IsValid(ThisSubsystem) ? ThisSubsystem->Manager : nullptr;
}

void UPrismatiscapeWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	SpawnManager();
}

void UPrismatiscapeWorldSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	DestroyManager(InWorld.GetWorld());
	Super::OnWorldEndPlay(InWorld);
}

void UPrismatiscapeWorldSubsystem::SpawnManager()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true;

//#if WITH_EDITOR
//	SpawnParams.bHideFromSceneOutliner = true;
//#endif

	Manager = GetWorld()->SpawnActor<APrismatiscapeManager>(UPrismatiscapeSettings::Get()->PrismatiscapeManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	UPrismatiscapeSettings::Get()->PrismatiscapeManager = Manager;
}

void UPrismatiscapeWorldSubsystem::DestroyManager(UWorld* InWorld)
{
	if (!Manager) return;
	if (!Get(InWorld)) return;

	if (Get(InWorld)->Manager)
	{
		UPrismatiscapeSettings::Get()->PrismatiscapeManager.Reset();
	}
}

bool UPrismatiscapeWorldSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
