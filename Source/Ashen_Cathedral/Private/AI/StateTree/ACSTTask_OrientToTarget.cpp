// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/StateTree/ACSTTask_OrientToTarget.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/KismetMathLibrary.h"

EStateTreeRunStatus FACSTTask_OrientToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	APawn* OwningPawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!OwningPawn || !InstanceData.TargetActor)
	{
		return EStateTreeRunStatus::Running;
	}

	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(OwningPawn->GetActorLocation(), InstanceData.TargetActor->GetActorLocation());
	LookAtRot.Pitch = 0.f;
	const FRotator TargetRot = FMath::RInterpTo(OwningPawn->GetActorRotation(), LookAtRot, DeltaTime, RotationInterpSpeed);

	OwningPawn->SetActorRotation(TargetRot);

	return EStateTreeRunStatus::Running;
}
