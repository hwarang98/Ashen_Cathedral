// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "ACGameplayTags.h"

void UACAbilitySystemComponent::OnAbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		if (AbilitySpec.IsActive())
		{
			// 이미 활성 중이면 InputPressed 전달 — 토글 어빌리티가 스스로 종료 결정
			AbilitySpecInputPressed(AbilitySpec);
		}
		else
		{
			TryActivateAbility(AbilitySpec.Handle);
		}
	}
}

void UACAbilitySystemComponent::OnAbilityInputReleased(const FGameplayTag& InputTag)
{
	// if (!InputTag.IsValid() || !InputTag.MatchesTag(ACGameplayTags::InputTag_MustBeHeld))
	// {
	// 	return;
	// }

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && AbilitySpec.IsActive())
		{
			// InputReleased 이벤트를 어빌리티에 전달 (취소하지 않음)
			FGameplayAbilitySpec* MutableSpec = FindAbilitySpecFromHandle(AbilitySpec.Handle);
			if (MutableSpec)
			{
				AbilitySpecInputReleased(*MutableSpec);
			}
		}
	}
}