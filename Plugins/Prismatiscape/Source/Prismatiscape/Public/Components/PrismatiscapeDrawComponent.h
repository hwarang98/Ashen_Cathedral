// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PrismatiscapeDrawComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class PRISMATISCAPE_API UPrismatiscapeDrawComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPrismatiscapeDrawComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	bool bDrawByDefault = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	bool bIsVisible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	float ActorScale = 1.0f;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Prismatiscape")
	void ToggleDrawing(bool bEnabled);
};