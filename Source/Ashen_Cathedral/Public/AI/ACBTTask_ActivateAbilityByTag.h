// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ACBTTask_EnemyBase.h"
#include "ACBTTask_ActivateAbilityByTag.generated.h"

/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACBTTask_ActivateAbilityByTag : public UACBTTask_EnemyBase
{
	GENERATED_BODY()

public:
	UACBTTask_ActivateAbilityByTag();
	virtual void OnEnemyExecuteTask() override;

private:
	UPROPERTY(EditAnywhere, Category = "Ability", meta=(Categories = "Enemy.Ability"))
	FGameplayTagContainer AbilityTagToActivate;
};