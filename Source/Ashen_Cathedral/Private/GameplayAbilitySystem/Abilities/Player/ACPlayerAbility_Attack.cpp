// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Attack.h"
#include "ACGameplayDebugHelper.h"
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

UAnimMontage* UACPlayerAbility_Attack::SelectAttackMontage()
{
	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();
	if (!PlayerCharacter)
	{
		return Super::SelectAttackMontage();
	}

	int32& SharedCount = PlayerCharacter->SharedComboCount;
	if (SharedCount >= AttackMontages.Num())
	{
		SharedCount = 0;
	}

	// 베이스 클래스가 SelectAttackMontage 이후 CurrentComboCount를 따로 증가시키므로
	// 여기서 SharedCount만 증가시켜 두 카운트가 독립적으로 관리되도록 한다.
	UAnimMontage* Selected = AttackMontages[SharedCount++];
	return Selected;
}

void UACPlayerAbility_Attack::HandleComboComplete()
{
	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();
	if (!PlayerCharacter)
		return;

	if (UWorld* World = GetWorld())
	{
		// 공유 핸들을 사용하므로 다른 어빌리티가 건 타이머를 자동으로 교체한다.
		World->GetTimerManager().SetTimer(
			PlayerCharacter->SharedComboResetTimerHandle,
			this,
			&ThisClass::OnComboResetTimerExpired,
			PlayerCharacter->ComboResetDelay,
			false);
	}
}

void UACPlayerAbility_Attack::HandleComboCancelled()
{
	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();
	if (PlayerCharacter)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PlayerCharacter->SharedComboResetTimerHandle);
		}
	}

	if (bComboChaining)
	{
		bComboChaining = false;
		return;
	}

	if (PlayerCharacter)
	{
		PlayerCharacter->SharedComboCount = 0;
	}
	Super::HandleComboCancelled();
}

void UACPlayerAbility_Attack::OnComboResetTimerExpired()
{
	if (AACPlayerCharacter* PlayerChar = GetTypedOuter<AACPlayerCharacter>())
	{
		PlayerChar->SharedComboCount = 0;
	}
	ResetComboCount();
}

void UACPlayerAbility_Attack::TriggerComboChain(const FGameplayTag& InputTag)
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
			if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag) || Spec.IsActive())
			{
				continue;
			}

			ASC->TryActivateAbility(Spec.Handle);
			break;
		}
	}
}