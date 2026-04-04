// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Attack.h"
#include "ACGameplayTags.h"
#include "Character/ACCharacterBase.h"
#include "Character/Player/ACPlayerCharacter.h"
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

	// TODO: 패링 윈도우 활성화 로직 구현 예정
	// TODO: Shared_Status_CanCounterAttack 태그 부여 로직 구현 예정
}

void UACPlayerAbility_Attack::HandleComboCancelled()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	}

	Super::HandleComboCancelled();
}

void UACPlayerAbility_Attack::OnComboResetTimerExpired()
{
	ResetComboCount();
}