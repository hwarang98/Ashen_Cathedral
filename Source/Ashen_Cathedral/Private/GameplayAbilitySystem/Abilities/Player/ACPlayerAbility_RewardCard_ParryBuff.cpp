// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_RewardCard_ParryBuff.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACPlayerAbility_RewardCard_ParryBuff::UACPlayerAbility_RewardCard_ParryBuff()
{
	// 부여 즉시 자동 활성화 — Passive GA
	AbilityActivationPolicy = EACAbilityActivationPolicy::OnGiven;

	FGameplayTagContainer OwnTags;
	OwnTags.AddTag(ACGameplayTags::Player_Ability_RewardCard_ParryBuff);
	SetAssetTags(OwnTags);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACPlayerAbility_RewardCard_ParryBuff::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	StartWaitingForParryTag();
}

void UACPlayerAbility_RewardCard_ParryBuff::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (WaitTagTask)
	{
		WaitTagTask->EndTask();
		WaitTagTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACPlayerAbility_RewardCard_ParryBuff::StartWaitingForParryTag()
{
	// 이전 Task 정리
	if (WaitTagTask)
	{
		WaitTagTask->EndTask();
		WaitTagTask = nullptr;
	}

	// Shared.Status.CanCounterAttack 태그 추가를 감지 — 패링 성공 판정 프록시로 사용
	WaitTagTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this, ACGameplayTags::Shared_Status_CanCounterAttack);

	WaitTagTask->Added.AddDynamic(this, &ThisClass::OnParryTagAdded);
	WaitTagTask->ReadyForActivation();
}

void UACPlayerAbility_RewardCard_ParryBuff::OnParryTagAdded()
{
	// ParryAttackBuffEffect가 설정된 경우 플레이어 ASC에 적용
	if (ParryAttackBuffEffect)
	{
		UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

			const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
				ParryAttackBuffEffect, GetAbilityLevel(), ContextHandle);

			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 다음 패링을 위해 다시 대기
	StartWaitingForParryTag();
}
