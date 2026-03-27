// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Sprint.h"
#include "ACGameplayTags.h"
#include "Character/ACCharacterBase.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"

UACAbility_Sprint::UACAbility_Sprint()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Sprint);
	SetAssetTags(TagsToAdd);

	AbilityActivationPolicy = EACAbilityActivationPolicy::OnTriggered;

	// 활성화 중 부여할 태그 -> AnimInstance 의 IsSprinting 갱신에 사용
	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_Sprinting);

	// 사망, 그로기 상태에서 Sprint 차단
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Groggy);
}

bool UACAbility_Sprint::CanActivateAbility(
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

	const AACCharacterBase* CharacterBase = ActorInfo ? Cast<AACCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!CharacterBase)
	{
		return false;
	}

	// 정지 상태에서 Sprint 진입 불가
	if (CharacterBase->GetVelocity().SizeSquared2D() < SprintStopThreshold * SprintStopThreshold)
	{
		return false;
	}

	// 스태미나 소진 상태에서 Sprint 진입 불가
	if (const UACAttributeSet* AttributeSet = CharacterBase->GetACAttributeSet())
	{
		if (AttributeSet->GetStamina() <= 0.f)
		{
			return false;
		}
	}

	return true;
}

void UACAbility_Sprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 코스트/쿨타임 적용 실패 시 능력 즉시 종료
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// SprintSpeed 값으로 Dynamic GE 생성 — MoveSpeed 를 Override -> PostAttributeChange 에서 MaxWalkSpeed 자동 동기화
	{
		UGameplayEffect* DynamicGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("SprintSpeed_DynamicGE"));
		DynamicGE->DurationPolicy = EGameplayEffectDurationType::Infinite;

		FGameplayModifierInfo& ModInfo = DynamicGE->Modifiers.AddDefaulted_GetRef();
		ModInfo.Attribute = UACAttributeSet::GetMoveSpeedAttribute();
		ModInfo.ModifierOp = EGameplayModOp::Override;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(SprintSpeed));

		SprintSpeedEffectHandle = ASC->ApplyGameplayEffectToSelf(DynamicGE, 1.f, ASC->MakeEffectContext());
	}

	// 스태미나 드레인 GE 적용 — 적(Enemy)은 nullptr 이므로 건너뜀
	// 드레인 GE 가 있을 때만 소진 감시 델리게이트도 등록
	if (SprintStaminaDrainEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(SprintStaminaDrainEffectClass);
		DrainEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

		StaminaDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UACAttributeSet::GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);
	}
}

void UACAbility_Sprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		// Sprint 속도 GE 제거 — MoveSpeed 복원 → PostAttributeChange 에서 MaxWalkSpeed 자동 동기화
		ASC->RemoveActiveGameplayEffect(SprintSpeedEffectHandle);

		// 드레인 GE 제거
		ASC->RemoveActiveGameplayEffect(DrainEffectHandle);

		// 스태미나 변화 델리게이트 해제
		ASC->GetGameplayAttributeValueChangeDelegate(UACAttributeSet::GetStaminaAttribute()).Remove(StaminaDelegateHandle);

		// Sprint 종료 후 즉시 회복 방지 — 딜레이 GE 적용
		if (ASC->StaminaRegenDelayEffectClass)
		{
			const UGameplayEffect* StaminaRegenDelayEffect = ASC->StaminaRegenDelayEffectClass->GetDefaultObject<UGameplayEffect>();
			ASC->ApplyGameplayEffectToSelf(StaminaRegenDelayEffect, 1, ASC->MakeEffectContext());
		}
	}

	SprintSpeedEffectHandle.Invalidate();
	DrainEffectHandle.Invalidate();
	StaminaDelegateHandle.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACAbility_Sprint::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UACAbility_Sprint::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}