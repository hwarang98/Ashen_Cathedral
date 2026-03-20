// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "PlayerCombatComponent.generated.h"

class AACWeapon;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UPlayerCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	UPlayerCombatComponent();

	AACWeapon* GetPlayerCarriedWeaponByTag(FGameplayTag InWeaponTag) const;
};