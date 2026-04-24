// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/ACCharacterBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Controllers/ACEnemyController.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Runtime/Media/Public/IMediaControls.h"

void UACGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (AbilityActivationPolicy == EACAbilityActivationPolicy::OnGiven)
	{
		// 어빌리티가 아직 활성화되지 않았다면
		if (ActorInfo && !Spec.IsActive())
		{
			// 어빌리티 활성화를 시도 
			// 이 함수는 내부적으로 CanActivateAbility를 호출하므로 서버/클라 권한 체크가 자동으로 수행
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

UACAbilitySystemComponent* UACGameplayAbility::GetACAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UACAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AACCharacterBase* UACGameplayAbility::GetACCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

AACPlayerCharacter* UACGameplayAbility::GetACPlayerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACPlayerCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

AACEnemyCharacter* UACGameplayAbility::GetACEnemyFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACEnemyCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UPawnCombatComponent* UACGameplayAbility::GetPawnCombatComponentFromActorInfo() const
{
	if (const AACCharacterBase* CharacterBase = GetACCharacterFromActorInfo())
	{
		return CharacterBase->GetPawnCombatComponent();
	}

	return nullptr;
}