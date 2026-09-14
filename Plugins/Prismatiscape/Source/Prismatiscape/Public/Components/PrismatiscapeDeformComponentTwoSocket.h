// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrismatiscapeDeformComponent.h"
#include "Components/MeshComponent.h"
#include "PrismatiscapeDeformComponentTwoSocket.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PRISMATISCAPE_API UPrismatiscapeDeformComponentTwoSocket : public UPrismatiscapeDeformComponent
{
	GENERATED_BODY()

public:
	UPrismatiscapeDeformComponentTwoSocket();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void SetMeshComponent();

	UFUNCTION(BlueprintNativeEvent, Category = "Prismatiscape")
	void CalculateVelocity();
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Default")
	TObjectPtr<UMeshComponent> MeshComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	FName SocketNameStart = "Butt";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	FName SocketNameEnd = "Tip";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float StartRadius = 10.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float EndRadius = 10.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float StartStrength = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float EndStrength = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Default")
	FVector CurrentStartSocketLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Default")
	FVector CurrentEndSocketLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Default")
	FVector CurrentStartVelocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Default")
	FVector CurrentEndVelocity = FVector::ZeroVector;

	virtual void GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength) override;
};