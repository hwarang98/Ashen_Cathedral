// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrismatiscapeTypes.generated.h"

USTRUCT(BlueprintType, Category = "Prismatiscape")
struct FPrismatiscapeBoneChainData
{
	GENERATED_BODY()

	FPrismatiscapeBoneChainData() {}
	FPrismatiscapeBoneChainData(const TArray<FName>& InBoneNames, const TArray<float>& InRadii) : BoneNamesRootToTip(InBoneNames), RadiiRootToTip(InRadii) {}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	TArray<FName> BoneNamesRootToTip;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	TArray<float> RadiiRootToTip;
};

USTRUCT(BlueprintType, Category = "Prismatiscape")
struct FPrismatiscapeBoneProfileData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	TArray<FPrismatiscapeBoneChainData> BoneChains;
};

USTRUCT(BlueprintType, Category = "Prismatiscape")
struct FPrismatiscapeBoneChainNameArray
{
	GENERATED_BODY()

	FPrismatiscapeBoneChainNameArray() {}
	FPrismatiscapeBoneChainNameArray(const TArray<FName>& InBoneChainNames, float InStrength) : BoneChainNames(InBoneChainNames), StrengthMultiplier(InStrength) {}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	TArray<FName> BoneChainNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	float StrengthMultiplier = 1.0f;
};

USTRUCT(BlueprintType, Category = "Prismatiscape")
struct FPrismatiscapeDrawShape
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	FVector4 StartLocationAndRadius = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	FVector4 EndLocationAndRadius = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	FVector4 StartVelocityAndStrength = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Prismatiscape")
	FVector4 EndVelocityAndStrength = FVector::ZeroVector;
};