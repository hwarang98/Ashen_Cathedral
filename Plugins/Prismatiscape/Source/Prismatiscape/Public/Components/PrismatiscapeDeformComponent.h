// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrismatiscapeDrawComponent.h"
#include "PrismatiscapeDeformComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PRISMATISCAPE_API UPrismatiscapeDeformComponent : public UPrismatiscapeDrawComponent
{
	GENERATED_BODY()

public:
	UPrismatiscapeDeformComponent();

protected:
	virtual void BeginPlay() override;

	virtual void ToggleDrawing_Implementation(bool bEnabled) override;

	/** Can be set to true avoid unnecessary calls to BP functions if this class is only going to be used in c++ */
	bool bSkipBlueprintGatherDeformCapsules = false;

public:
	/** Override to *add* any deform capsules to the simulation this frame */
	UFUNCTION(BlueprintCallable, Category = "Prismatiscape")
	virtual void GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength);

	UFUNCTION(BlueprintImplementableEvent, Category = "Prismatiscape", DisplayName="Get Deform Capsules")
	void BP_GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength);
};
