// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/StateTree/ACSTTask_SendGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeAsyncExecutionContext.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "TimerManager.h"

FACSTTask_SendGameplayEvent::FACSTTask_SendGameplayEvent()
{
	// 종료 감지를 델리게이트로 처리하므로 틱이 필요 없다
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FACSTTask_SendGameplayEvent::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AACEnemyCharacter* EnemyCharacter = InstanceData.AIController ? Cast<AACEnemyCharacter>(InstanceData.AIController->GetPawn()) : nullptr;
	if (!EnemyCharacter || !EventTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 좌/우 랜덤이면 Left(2)/Right(3) 중 하나를, 아니면 지정된 방향을 EventMagnitude로 보낸다
	const EACDodgeDirection Direction = bRandomLeftRight
		? (FMath::RandBool() ? EACDodgeDirection::Left : EACDodgeDirection::Right)
		: DodgeDirection;

	FGameplayEventData Payload;
	Payload.EventMagnitude = static_cast<float>(Direction);
	Payload.Instigator = EnemyCharacter;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(EnemyCharacter, EventTag, Payload);

	// WaitOwnedTag가 없으면 발송 즉시 완료한다
	if (!WaitOwnedTag.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	UAbilitySystemComponent* ASC = EnemyCharacter->GetACAbilitySystemComponent();
	if (!ASC)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 이벤트로 켜진 어빌리티가 아직 태그를 부여하지 않았다면(즉시 실패 등) 더 기다릴 필요가 없다
	if (!ASC->HasMatchingGameplayTag(WaitOwnedTag))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.AbilitySystemComponent = ASC;

	FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();

	InstanceData.TagChangedHandle = ASC->RegisterGameplayTagEvent(WaitOwnedTag, EGameplayTagEventType::NewOrRemoved).AddLambda(
		[WeakContext](const FGameplayTag Tag, int32 NewCount)
		{
			// 태그가 사라진 시점이 동작 종료
			if (NewCount <= 0)
			{
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		});

	if (MaxWaitTime > 0.f)
	{
		if (UWorld* World = EnemyCharacter->GetWorld())
		{
			World->GetTimerManager().SetTimer(InstanceData.TimeoutTimerHandle, FTimerDelegate::CreateLambda(
				[WeakContext]()
				{
					WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
				}), MaxWaitTime, false);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FACSTTask_SendGameplayEvent::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	CleanupWait(Context.GetInstanceData(*this));
}

void FACSTTask_SendGameplayEvent::CleanupWait(FInstanceDataType& InstanceData) const
{
	if (UAbilitySystemComponent* ASC = InstanceData.AbilitySystemComponent.Get())
	{
		if (InstanceData.TagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(WaitOwnedTag, EGameplayTagEventType::NewOrRemoved).Remove(InstanceData.TagChangedHandle);
		}
	}

	if (const AAIController* AIController = InstanceData.AIController)
	{
		if (const UWorld* World = AIController->GetWorld())
		{
			World->GetTimerManager().ClearTimer(InstanceData.TimeoutTimerHandle);
		}
	}

	InstanceData.TagChangedHandle.Reset();
	InstanceData.AbilitySystemComponent = nullptr;
}
