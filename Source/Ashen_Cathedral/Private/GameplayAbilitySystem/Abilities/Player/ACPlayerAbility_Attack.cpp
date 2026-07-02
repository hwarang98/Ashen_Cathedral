// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Attack.h"
#include "ACGameplayTags.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"

UACPlayerAbility_Attack::UACPlayerAbility_Attack()
{
	// 이 어빌리티의 식별 태그 (에셋 태그)
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Attack_Light);
	SetAssetTags(TagsToAdd);

	// 이 어빌리티가 활성화된 동안 아래 태그를 가진 어빌리티는 활성화 불가
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Attack_Light);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_EquipWeapon);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_UnEquipWeapon);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);

	// 공격 시작 시 진행 중인 스프린트를 즉시 취소
	CancelAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);
}

void UACPlayerAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bComboChaining = false;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool UACPlayerAbility_Attack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AACPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<AACPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UACAttributeSet* AttributeSet = PlayerCharacter ? PlayerCharacter->GetACAttributeSet() : nullptr;
	if (AttributeSet && AttributeSet->GetStamina() <= 0.f)
	{
		return false;
	}

	return true;
}

void UACPlayerAbility_Attack::HandleComboComplete()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ComboResetTimerHandle,
			this,
			&ThisClass::OnComboResetTimerExpired,
			ComboResetDelay,
			false);
	}
}

void UACPlayerAbility_Attack::HandleComboCancelled()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	}

	if (bComboChaining)
	{
		bComboChaining = false;
		return;
	}

	Super::HandleComboCancelled();
}

void UACPlayerAbility_Attack::OnComboResetTimerExpired()
{
	ResetComboCount();
}

void UACPlayerAbility_Attack::TriggerComboChain()
{
	if (!IsActive())
	{
		return;
	}

	// EndAbility 이후 CurrentActorInfo가 무효화될 수 있으므로 미리 ASC를 캐시한다.
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	// 콤보 윈도우 구간(ANS_ComboWindow)이 아니면 체인을 허용하지 않는다.
	if (!ASC || !ASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_ComboWindow))
	{
		return;
	}

	bComboChaining = true;
	// EndAbility(true) → HandleComboCancelled(bComboChaining=true → ResetComboCount 건너뜀)
	//                   → Super::EndAbility() → bIsActive=false, 블록 해제
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

	// EndAbility 완료 직후 동기 방식으로 다음 어빌리티를 활성화한다.
	// PlayMontageAndWait가 새 몽타주를 재생하면 기존 몽타주는 자동으로 블렌드아웃된다.
	if (IsValid(ASC))
	{
		for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.GetDynamicSpecSourceTags().HasTagExact(ACGameplayTags::InputTag_LightAttack) || Spec.IsActive())
			{
				continue;
			}

			ASC->TryActivateAbility(Spec.Handle);
			break;
		}
	}
}