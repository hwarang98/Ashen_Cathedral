// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrismatiscapeManager.generated.h"

UCLASS()
class PRISMATISCAPE_API APrismatiscapeManager : public AActor
{
	GENERATED_BODY()

public:
	APrismatiscapeManager();

	static APrismatiscapeManager* Get(const UObject* WorldContextObject);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Prismatiscape", meta=(ForceAsFunction))
	void SetFollowLocationThisFrame();

	UFUNCTION(BlueprintNativeEvent, Category = "Prismatiscape", meta=(ForceAsFunction))
	void UpdateAllVisibilities();

	UFUNCTION(BlueprintNativeEvent, Category = "Prismatiscape", meta=(ForceAsFunction))
	void FlushAllArrays();

	UFUNCTION(BlueprintNativeEvent, Category = "Prismatiscape", meta=(ForceAsFunction))
	void GatherAllShapesFromRegisteredComponents();

	UFUNCTION(BlueprintImplementableEvent, Category = "Prismatiscape")
	void PostTick();

	UFUNCTION(BlueprintNativeEvent, Category = "Prismatiscape", meta=(ForceAsFunction))
	void DrawDebugShapes();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	class USceneComponent* ComponentToFollow = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	FVector FollowLocationThisFrame = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<class UPrismatiscapeDeformComponent*> DeformComponents;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> CapsuleStartLocationAndRadius;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> CapsuleEndLocationAndRadius;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> CapsuleStartVelocityAndStrength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> CapsuleEndVelocityAndStrength;
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<class UPrismatiscapeWindComponent*> WindComponents;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> WindCapsuleStartLocationAndRadius;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> WindCapsuleEndLocationAndRadius;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> WindCapsuleStartVelocityAndStrength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> WindCapsuleEndVelocityAndStrength;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<class UPrismatiscapeInteractionBubbleComponent*> InteractionBubbleComponents;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> InteractionBubbleLocationAndRadius;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Arrays")
	TArray<FVector4> InteractionBubbleVelocityAndStrength;
};
