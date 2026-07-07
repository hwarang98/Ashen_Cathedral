// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ACBTTask_ActivateAbilityByTagAndWait.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Structs/ACStructTypes.h"

UACBTTask_ActivateAbilityByTagAndWait::UACBTTask_ActivateAbilityByTagAndWait()
{
	NodeName = TEXT("BTTask_ActivateAbilityByTagAndWait");

	// TickTask() 콜백을 활성화
	bNotifyTick = true;

	// 노드 인스턴스를 AI마다 따로 생성하지 않고, NodeMemory로 상태를 분리 -> 여러 AI가 같은 태스크를 동시에 실행해도 서로 간섭 없음
	bCreateNodeInstance = false;

	// bNotifyTick 등의 플래그를 실제 내부 비트 필드에 반영
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UACBTTask_ActivateAbilityByTagAndWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FActivateAbilityAndWaitTaskMemory* Memory = CastInstanceNodeMemory<FActivateAbilityAndWaitTaskMemory>(NodeMemory);
	check(Memory);

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;

	AACEnemyCharacter* EnemyCharacter = Cast<AACEnemyCharacter>(ControlledPawn);
	if (!EnemyCharacter)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* AbilitySystemComponent = EnemyCharacter->GetACAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		return EBTNodeResult::Failed;
	}

	FGameplayAbilitySpecHandle FoundHandle;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasAll(AbilityTagToActivate))
		{
			FoundHandle = Spec.Handle;
			break;
		}
	}

	if (!FoundHandle.IsValid())
	{
		return bFailIfAbilityNotActivated ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}

	if (bStopMovementBeforeActivate)
	{
		if (AAIController* MutableAIController = OwnerComp.GetAIOwner())
		{
			MutableAIController->StopMovement();
		}
	}

	if (!AbilitySystemComponent->TryActivateAbility(FoundHandle))
	{
		return bFailIfAbilityNotActivated ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}

	Memory->OwningEnemyCharacter = EnemyCharacter;
	Memory->AbilitySystemComponent = AbilitySystemComponent;
	Memory->AbilityHandle = FoundHandle;
	Memory->ElapsedTime = 0.f;

	// 활성화 직후 이미 종료된 경우(즉시 완료형 어빌리티) TickTask 없이 바로 성공 처리
	if (!IsAbilityStillRunning(*Memory))
	{
		Memory->Reset();
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UACBTTask_ActivateAbilityByTagAndWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FActivateAbilityAndWaitTaskMemory* Memory = CastInstanceNodeMemory<FActivateAbilityAndWaitTaskMemory>(NodeMemory);

	if (!Memory->IsValid())
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!IsAbilityStillRunning(*Memory))
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	Memory->ElapsedTime += DeltaSeconds;
	if (MaxWaitTime > 0.f && Memory->ElapsedTime >= MaxWaitTime)
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, bSucceedOnTimeout ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
}

uint16 UACBTTask_ActivateAbilityByTagAndWait::GetInstanceMemorySize() const
{
	return sizeof(FActivateAbilityAndWaitTaskMemory);
}

FString UACBTTask_ActivateAbilityByTagAndWait::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s 태그의 Ability를 활성화하고 종료될 때까지 대기"), *AbilityTagToActivate.ToStringSimple());
}

bool UACBTTask_ActivateAbilityByTagAndWait::IsAbilityStillRunning(const FActivateAbilityAndWaitTaskMemory& Memory) const
{
	UAbilitySystemComponent* AbilitySystemComponent = Memory.AbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return false;
	}

	if (WaitOwnedTag.IsValid())
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(WaitOwnedTag);
	}

	const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Memory.AbilityHandle);
	return Spec && Spec->IsActive();
}
