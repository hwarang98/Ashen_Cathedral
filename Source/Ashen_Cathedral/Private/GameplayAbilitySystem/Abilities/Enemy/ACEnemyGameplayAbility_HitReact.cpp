// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility_HitReact.h"

#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UACEnemyGameplayAbility_HitReact::UACEnemyGameplayAbility_HitReact()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Shared_Ability_HitReact);
	SetAssetTags(TagsToAdd);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ACGameplayTags::Shared_Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UACEnemyGameplayAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const AActor* InstigatorActor = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	UAnimMontage* MontageToPlay = FrontHitReactMontage;

	if (InstigatorActor && AvatarActor)
	{
		float AngleDifference = 0.f;
		// 공격자와의 각도를 계산하여 방향 태그(Front, Left, Right, Back)를 반환받음
		const FGameplayTag HitDirectionTag = UACFunctionLibrary::ComputeHitReactDirectionTag(InstigatorActor, AvatarActor, AngleDifference);

		if (HitDirectionTag.MatchesTagExact(ACGameplayTags::Shared_Status_HitReact_Left))
		{
			MontageToPlay = LeftHitReactMontage;
		}
		else if (HitDirectionTag.MatchesTagExact(ACGameplayTags::Shared_Status_HitReact_Right))
		{
			MontageToPlay = RightHitReactMontage;
		}
		else if (HitDirectionTag.MatchesTagExact(ACGameplayTags::Shared_Status_HitReact_Back))
		{
			MontageToPlay = BackHitReactMontage;
		}
	}

	if (MontageToPlay)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			MontageToPlay,
			1.0f,
			NAME_None,
			false,
			1.0f
			);

		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, MakeOutgoingGameplayEffectSpec(UnderAttackEffect, 1));

		MontageTask->ReadyForActivation();
	}
	else
	{
		// 재생할 몽타주가 없으면 즉시 종료
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UACEnemyGameplayAbility_HitReact::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACEnemyGameplayAbility_HitReact::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}