// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ACBTTask_EnemyBase.h"
#include "AIController.h"
#include "Character/Enemy/ACEnemyCharacter.h"

UACBTTask_EnemyBase::UACBTTask_EnemyBase()
{
	NodeName = TEXT("Enemy Base Task");
}

EBTNodeResult::Type UACBTTask_EnemyBase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	// 이미 캐싱된 경우
	if (!IsValid(OwningEnemyCharacter))
	{
		OwningEnemyCharacter = Cast<AACEnemyCharacter>(ControlledPawn);

		if (!OwningEnemyCharacter)
		{
			return EBTNodeResult::Failed;
		}
	}

	// BP의 OnEnemyExecuteTask
	OnEnemyExecuteTask();

	return EBTNodeResult::Succeeded;
}

void UACBTTask_EnemyBase::OnEnemyExecuteTask() {}