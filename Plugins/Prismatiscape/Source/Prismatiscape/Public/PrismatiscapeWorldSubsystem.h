// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PrismatiscapeWorldSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PRISMATISCAPE_API UPrismatiscapeWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UPrismatiscapeWorldSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Prismatiscape", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static class APrismatiscapeManager* GetPrismatiscapeManager(const UObject* WorldContextObject);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	virtual void OnWorldEndPlay(UWorld& InWorld) override;

	UFUNCTION()
	void SpawnManager();
	UFUNCTION()
	void DestroyManager(UWorld* InWorld);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	
private:
	UPROPERTY()
	TObjectPtr<class APrismatiscapeManager> Manager;
};