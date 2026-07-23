// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/StateTree/ACSTTask_UpdateWarpTarget.h"
#include "AIController.h"
#include "MotionWarpingComponent.h"
#include "StateTreeExecutionContext.h"
#include "Character/ACCharacterBase.h"

EStateTreeRunStatus FACSTTask_UpdateWarpTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 진입 즉시 한 번 갱신해 첫 프레임부터 유효한 워프 타겟이 있도록 한다
	InstanceData.TimeSinceLastUpdate = 0.f;
	UpdateWarpTarget(InstanceData);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FACSTTask_UpdateWarpTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.TimeSinceLastUpdate += DeltaTime;
	if (InstanceData.TimeSinceLastUpdate >= UpdateInterval)
	{
		InstanceData.TimeSinceLastUpdate = 0.f;
		UpdateWarpTarget(InstanceData);
	}

	return EStateTreeRunStatus::Running;
}

void FACSTTask_UpdateWarpTarget::UpdateWarpTarget(const FInstanceDataType& InstanceData) const
{
	const AACCharacterBase* OwningCharacter = InstanceData.AIController ? Cast<AACCharacterBase>(InstanceData.AIController->GetPawn()) : nullptr;
	if (!OwningCharacter || !InstanceData.TargetActor)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarping = OwningCharacter->GetMotionWarpingComponent();
	if (!MotionWarping)
	{
		return;
	}

	const FVector TargetLocation = InstanceData.TargetActor->GetActorLocation();

	FRotator TargetRotation = (TargetLocation - OwningCharacter->GetActorLocation()).Rotation();
	TargetRotation.Pitch = 0.f;
	TargetRotation.Roll = 0.f;

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
}
