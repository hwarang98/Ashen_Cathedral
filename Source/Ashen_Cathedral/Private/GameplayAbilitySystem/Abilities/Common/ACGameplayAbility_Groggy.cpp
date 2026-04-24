// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Common/ACGameplayAbility_Groggy.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/ACCharacterBase.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"

UACGameplayAbility_Groggy::UACGameplayAbility_Groggy()
{
	// 에셋 식별 태그 — CancelAbilities 에서 자신을 제외할 때 사용
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Shared_Ability_Groggy);
	SetAssetTags(TagsToAdd);

	// 어빌리티 활성 중 Shared_Status_Groggy 자동 부여, EndAbility 시 자동 제거
	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_Groggy);
	// 사망 상태에서는 그로기 진입 차단 (AttributeSet에서도 체크하지만 이중 안전망)
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Shared.Event.GroggyTriggered 이벤트를 받으면 자동 트리거
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ACGameplayTags::Shared_Event_GroggyTriggered;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UACGameplayAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AACCharacterBase* CharacterBase = GetACCharacterFromActorInfo();
	if (!CharacterBase)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 1. 이동 중단
	if (UCharacterMovementComponent* CharacterMovementComponent = CharacterBase->GetCharacterMovement())
	{
		CharacterMovementComponent->DisableMovement();
		CharacterMovementComponent->StopMovementImmediately();
	}

	// 2. 플레이어 입력 차단 (적에게는 PlayerController가 없으므로 자동 스킵됨)
	if (APlayerController* PlayerController = Cast<APlayerController>(CharacterBase->GetController()))
	{
		CharacterBase->DisableInput(PlayerController);
	}

	// 3. 몽타주 랜덤 선택
	UAnimMontage* SelectedMontage = nullptr;
	if (GroggyMontages.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, GroggyMontages.Num() - 1);
		SelectedMontage = GroggyMontages[RandomIndex];
	}

	if (!SelectedMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 5. 몽타주 재생 (자동 블렌드아웃 활성화)
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		SelectedMontage,
		1.0f,
		NAME_None,
		true,
		1.0f
		);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

void UACGameplayAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AACCharacterBase* CharacterBase = GetACCharacterFromActorInfo())
	{
		// 처형 중에는 이동·입력 복구 생략 — GA_Execution이 종료 시점에 직접 복구한다
		const UACAbilitySystemComponent* OwnerASC = GetACAbilitySystemComponentFromActorInfo();
		const bool bIsBeingExecuted = OwnerASC && OwnerASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Executed);

		if (!bIsBeingExecuted)
		{
			// 1. 이동 복구
			if (UCharacterMovementComponent* Movement = CharacterBase->GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Walking);
			}

			// 2. 플레이어 입력 복구
			if (APlayerController* PlayerController = Cast<APlayerController>(CharacterBase->GetController()))
			{
				CharacterBase->EnableInput(PlayerController);
			}
		}
	}

	// 3. GroggyGauge 초기화 — 그로기 종료 후 다음 누적을 0부터 다시 시작
	if (const UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		if (UACAttributeSet* AttributeSet = const_cast<UACAttributeSet*>(ASC->GetSet<UACAttributeSet>()))
		{
			AttributeSet->SetGroggyGauge(0.f);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACGameplayAbility_Groggy::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACGameplayAbility_Groggy::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}