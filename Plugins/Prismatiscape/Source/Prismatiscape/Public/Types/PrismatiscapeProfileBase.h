// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PrismatiscapeProfileBase.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Abstract)
class PRISMATISCAPE_API UPrismatiscapeProfileBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Prismatiscape Profile")
	void OnPropertyChange(FName PropertyName);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};