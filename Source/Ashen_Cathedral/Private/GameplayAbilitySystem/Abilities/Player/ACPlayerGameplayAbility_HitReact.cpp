// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility_HitReact.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACPlayerGameplayAbility_HitReact::UACPlayerGameplayAbility_HitReact()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Shared_Ability_HitReact);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_HitReact);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ACGameplayTags::Shared_Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UACPlayerGameplayAbility_HitReact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const AActor* InstigatorActor = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	const bool bIsBlocking = GetACAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(ACGameplayTags::Player_Status_Blocking);

	UAnimMontage* MontageToPlay = bIsBlocking ? BlockHitReactMontage : FrontHitReactMontage;

	if (InstigatorActor && AvatarActor)
	{
		float AngleDifference = 0.f;
		const FGameplayTag HitDirectionTag = UACFunctionLibrary::ComputeHitReactDirectionTag(InstigatorActor, AvatarActor, AngleDifference);

		if (HitDirectionTag.MatchesTagExact(ACGameplayTags::Shared_Status_HitReact_Left))
		{
			MontageToPlay = bIsBlocking ? BlockHitReactMontage : LeftHitReactMontage;
		}
		else if (HitDirectionTag.MatchesTagExact(ACGameplayTags::Shared_Status_HitReact_Right))
		{
			MontageToPlay = bIsBlocking ? BlockHitReactMontage : RightHitReactMontage;
		}
		else if (HitDirectionTag.MatchesTagExact(ACGameplayTags::Shared_Status_HitReact_Back))
		{
			MontageToPlay = bIsBlocking ? BlockHitReactMontage : BackHitReactMontage;
		}
	}

	if (HitCameraShakeClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
		{
			PlayerController->ClientStartCameraShake(HitCameraShakeClass);
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
			1.0f);

		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, MakeOutgoingGameplayEffectSpec(UnderAttackEffect, 1));

		MontageTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UACPlayerGameplayAbility_HitReact::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACPlayerGameplayAbility_HitReact::OnMontageCancelled()
{

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}