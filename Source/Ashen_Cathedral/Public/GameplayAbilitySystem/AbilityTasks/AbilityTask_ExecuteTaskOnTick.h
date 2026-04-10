// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_ExecuteTaskOnTick.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityTaskTickDelegate, float, DeltaTime);

/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UAbilityTask_ExecuteTaskOnTick : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_ExecuteTaskOnTick();

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|AbilityTask", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_ExecuteTaskOnTick* ExecuteTaskOnTick(UGameplayAbility* OwningAbility);

	virtual void TickTask(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "Ashen Cathdral|AbilityTask")
	FOnAbilityTaskTickDelegate OnAbilityTaskTick;
};