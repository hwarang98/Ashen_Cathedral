// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/UI/PlayerUIComponent.h"
#include "UObject/Interface.h"
#include "PawnUIInterface.generated.h"

class UEnemyUIComponent;
class UPawnUIComponent;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPawnUIInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ASHEN_CATHEDRAL_API IPawnUIInterface
{
	GENERATED_BODY()

public:
	virtual UPawnUIComponent* GetPawnUIComponent() const = 0;
	virtual UPlayerUIComponent* GetPlayerUIComponent() const;
	virtual UEnemyUIComponent* GetEnemyUIComponent() const;
};