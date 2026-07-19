// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Common/ACGameplayAbility_Block.h"

#include "AbilitySystemComponent.h"
#include "ACGameplayTags.h"

UACGameplayAbility_Block::UACGameplayAbility_Block()
{
	// Blocking 상태 태그는 Player/Enemy가 서로 다르므로(Player.Status.Blocking / Enemy.Status.Blocking)
	// 공용 베이스가 아니라 각 서브클래스(UACPlayerAbility_Block / UACEnemyAbility_Block) 생성자에서 부여한다.
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_PostureBroken);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACGameplayAbility_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveGameplayEffect(MoveSpeedEffectHandle);
	}
	MoveSpeedEffectHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}