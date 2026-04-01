// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "ACBTTask_EnemyBase.generated.h"

class AACEnemyCharacter;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACBTTask_EnemyBase : public UBTTask_BlueprintBase
{
	GENERATED_BODY()

public:
	UACBTTask_EnemyBase();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY()
	TObjectPtr<AACEnemyCharacter> OwningEnemyCharacter;

	virtual void OnEnemyExecuteTask();
};