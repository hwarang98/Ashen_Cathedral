// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrismatiscapeDeformComponent.h"
#include "Types/PrismatiscapeTypes.h"
#include "Components/MeshComponent.h"
#include "PrismatiscapeDeformComponentCharacter.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PRISMATISCAPE_API UPrismatiscapeDeformComponentCharacter : public UPrismatiscapeDeformComponent
{
	GENERATED_BODY()

public:
	UPrismatiscapeDeformComponentCharacter();

	virtual void BeginPlay() override;
	virtual void SetSkeletalMesh();

	virtual void GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength) override;

	UFUNCTION(BlueprintCallable, Category = "Prismatiscape")
	FPrismatiscapeBoneChainNameArray GetActiveBoneProfile();

	UFUNCTION(BlueprintCallable, Category = "Prismatiscape")
	void SetActiveProfile(FName NewProfileName);

	UPROPERTY(BlueprintReadWrite, Category = "Default")
	TObjectPtr<class UMeshComponent> SkeletalMesh = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	FName ActiveProfile = "FullBody";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	TMap<FName, FPrismatiscapeBoneChainData> BoneChains;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	TMap<FName, FPrismatiscapeBoneChainNameArray> BoneProfiles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float DrawStrength = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	TArray<FVector> PrevFramePositions;


};