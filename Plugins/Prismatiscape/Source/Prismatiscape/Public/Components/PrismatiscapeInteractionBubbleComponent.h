// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrismatiscapeDrawComponent.h"
#include "PrismatiscapeInteractionBubbleComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PRISMATISCAPE_API UPrismatiscapeInteractionBubbleComponent : public UPrismatiscapeDrawComponent
{
	GENERATED_BODY()

public:
	UPrismatiscapeInteractionBubbleComponent();

protected:
	bool bSkipBlueprintGatherInteractionBubbles = false;

	virtual void BeginPlay() override;
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float Radius = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float HeightOffset = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Prismatiscape")
	virtual void GetInteractionBubbles(TArray<FVector4>& LocationAndRadius, TArray<FVector4>& VelocityAndStrength);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Prismatiscape", DisplayName="Get Interaction Bubbles")
	void BP_GetInteractionBubbles(TArray<FVector4>& LocationAndRadius, TArray<FVector4>& VelocityAndStrength);

	virtual void ToggleDrawing_Implementation(bool bEnabled) override;
};
