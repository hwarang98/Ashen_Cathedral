// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/AbilityTasks/AbilityTask_ExecuteTaskOnTick.h"

UAbilityTask_ExecuteTaskOnTick::UAbilityTask_ExecuteTaskOnTick()
{
	bTickingTask = true;
}

UAbilityTask_ExecuteTaskOnTick* UAbilityTask_ExecuteTaskOnTick::ExecuteTaskOnTick(UGameplayAbility* OwningAbility)
{
	UAbilityTask_ExecuteTaskOnTick* Node = NewAbilityTask<UAbilityTask_ExecuteTaskOnTick>(OwningAbility);

	return Node;
}

void UAbilityTask_ExecuteTaskOnTick::TickTask(float DeltaSeconds)
{
	Super::TickTask(DeltaSeconds);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnAbilityTaskTick.Broadcast(DeltaSeconds);
	}
	else
	{
		EndTask();
	}
}