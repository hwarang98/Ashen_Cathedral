// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrismatiscapeDrawComponent.h"
#include "PrismatiscapeWindComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PRISMATISCAPE_API UPrismatiscapeWindComponent : public UPrismatiscapeDrawComponent
{
	GENERATED_BODY()

public:
	UPrismatiscapeWindComponent();

protected:
	virtual void BeginPlay() override;

	bool bSkipBlueprintGatherWindCapsules = false;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Prismatiscape")
	virtual void GetWindCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength);

	UFUNCTION(BlueprintImplementableEvent, Category = "Prismatiscape", DisplayName="Get Wind Capsules")
	void BP_GetWindCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength);

	virtual void ToggleDrawing_Implementation(bool bEnabled) override;
};