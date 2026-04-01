// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ACBTTask_EnemyBase.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "ACBTTask_ToggleStrafingState.generated.h"

/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACBTTask_ToggleStrafingState : public UACBTTask_EnemyBase
{
	GENERATED_BODY()

public:
	UACBTTask_ToggleStrafingState();

private:
	virtual void OnEnemyExecuteTask() override;

	UPROPERTY(EditAnywhere)
	bool ShouldEnable = false;

	UPROPERTY(EditAnywhere)
	bool ShouldChangeMaxWalkSpeed = false;

	UPROPERTY(EditAnywhere)
	float MaxWalkSpeed = 200.f;

	// Strafing 비활성화 시 기본 이동 속도를 읽어올 Blackboard 키
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector InDefaultMaxWalkSpeedKey;
};