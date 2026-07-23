// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/StateTree/ACSTTask_ActivateAbilityByTag.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeAsyncExecutionContext.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "TimerManager.h"

FACSTTask_ActivateAbilityByTag::FACSTTask_ActivateAbilityByTag()
{
	// 종료 감지를 델리게이트로 처리하므로 틱이 필요 없다
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FACSTTask_ActivateAbilityByTag::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const AACEnemyCharacter* EnemyCharacter = InstanceData.AIController ? Cast<AACEnemyCharacter>(InstanceData.AIController->GetPawn()) : nullptr;
	if (!EnemyCharacter)
	{
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = EnemyCharacter->GetACAbilitySystemComponent();
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayAbilitySpecHandle FoundHandle = FindAbilityHandle(*ASC);
	if (!FoundHandle.IsValid())
	{
		return bFailIfAbilityNotActivated ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Succeeded;
	}

	if (bStopMovementBeforeActivate)
	{
		InstanceData.AIController->StopMovement();
	}

	if (!ASC->TryActivateAbility(FoundHandle))
	{
		return bFailIfAbilityNotActivated ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Succeeded;
	}

	InstanceData.AbilitySystemComponent = ASC;
	InstanceData.AbilityHandle = FoundHandle;

	FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();

	// 종료 감지 수단은 둘 중 하나만 건다.
	// 어빌리티가 몽타주를 걸어놓고 바로 끝나는 형태(fire-and-forget)면 OnAbilityEnded가 재생 도중 발화하므로,
	// WaitOwnedTag가 지정된 경우에는 태그가 사라지는 시점만 종료로 본다
	if (WaitOwnedTag.IsValid())
	{
		InstanceData.TagChangedHandle = ASC->RegisterGameplayTagEvent(WaitOwnedTag, EGameplayTagEventType::NewOrRemoved).AddLambda(
			[WeakContext](const FGameplayTag Tag, int32 NewCount)
			{
				// 태그가 사라진 시점이 어빌리티 종료
				if (NewCount <= 0)
				{
					WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
				}
			});
	}
	else
	{
		const FGameplayAbilitySpecHandle WaitHandle = FoundHandle;
		InstanceData.AbilityEndedHandle = ASC->OnAbilityEnded.AddLambda(
			[WeakContext, WaitHandle](const FAbilityEndedData& EndedData)
			{
				if (EndedData.AbilitySpecHandle == WaitHandle)
				{
					WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
				}
			});
	}

	if (MaxWaitTime > 0.f)
	{
		if (UWorld* World = EnemyCharacter->GetWorld())
		{
			const bool bSucceed = bSucceedOnTimeout;
			World->GetTimerManager().SetTimer(InstanceData.TimeoutTimerHandle, FTimerDelegate::CreateLambda(
				[WeakContext, bSucceed]()
				{
					WeakContext.FinishTask(bSucceed ? EStateTreeFinishTaskType::Succeeded : EStateTreeFinishTaskType::Failed);
				}), MaxWaitTime, false);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FACSTTask_ActivateAbilityByTag::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	CleanupWait(Context.GetInstanceData(*this));
}

FGameplayAbilitySpecHandle FACSTTask_ActivateAbilityByTag::FindAbilityHandle(const UAbilitySystemComponent& ASC) const
{
	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasAll(AbilityTagToActivate))
		{
			return Spec.Handle;
		}
	}

	return FGameplayAbilitySpecHandle();
}

bool FACSTTask_ActivateAbilityByTag::IsAbilityStillRunning(const FInstanceDataType& InstanceData) const
{
	const UAbilitySystemComponent* ASC = InstanceData.AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	if (WaitOwnedTag.IsValid())
	{
		return ASC->HasMatchingGameplayTag(WaitOwnedTag);
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.AbilityHandle);
	return Spec && Spec->IsActive();
}

void FACSTTask_ActivateAbilityByTag::CleanupWait(FInstanceDataType& InstanceData) const
{
	if (UAbilitySystemComponent* ASC = InstanceData.AbilitySystemComponent.Get())
	{
		if (InstanceData.TagChangedHandle.IsValid() && WaitOwnedTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(WaitOwnedTag, EGameplayTagEventType::NewOrRemoved).Remove(InstanceData.TagChangedHandle);
		}

		if (InstanceData.AbilityEndedHandle.IsValid())
		{
			ASC->OnAbilityEnded.Remove(InstanceData.AbilityEndedHandle);
		}

		// 상태가 중단되어 나가는 경우 진행 중인 어빌리티도 함께 정리한다
		if (InstanceData.AbilityHandle.IsValid())
		{
			ASC->CancelAbilityHandle(InstanceData.AbilityHandle);
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
	InstanceData.AbilityEndedHandle.Reset();
	InstanceData.AbilitySystemComponent = nullptr;
	InstanceData.AbilityHandle = FGameplayAbilitySpecHandle();
}
