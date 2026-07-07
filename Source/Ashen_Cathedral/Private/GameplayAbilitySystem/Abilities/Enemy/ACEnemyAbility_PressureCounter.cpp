// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_PressureCounter.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACEnemyAbility_PressureCounter::UACEnemyAbility_PressureCounter()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Enemy_Ability_Pressure_Counter);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor);
	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_Invincible);
	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_Status_PressureCountering);

	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Groggy);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Executed);
	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_Dodging);

	// 압박 반격 발동 시 진행 중인 공격 어빌리티를 강제로 취소해 몽타주가 겹치지 않게 한다
	CancelAbilitiesWithTag.AddTag(ACGameplayTags::Enemy_Ability_Melee);

	// BT가 Enemy.State.PressureReady 태그를 보고 ACBTTask_ActivateAbilityByTag(Enemy.Ability.Pressure.Counter)로 직접 활성화한다

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACEnemyAbility_PressureCounter::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 압박 반응으로 선택되어 실제로 시작됨 — Detection이 다시 감지할 수 있도록 요청 태그를 즉시 해제한다
	UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(GetAvatarActorFromActorInfo(), ACGameplayTags::Enemy_State_PressureReady);

	if (!GetEnemyCharacterFromActorInfo())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CounterMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ApplyInvincibilityEffect();

	// 무기 콜리전(AnimNotifyState)이 겹치면 PawnCombatComponent가 Shared_Event_MeleeHit을 전송한다
	UAbilityTask_WaitGameplayEvent* WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Shared_Event_MeleeHit);
	WaitHitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitTarget);
	WaitHitTask->ReadyForActivation();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CounterMontage, 1.0f, NAME_None, false);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

void UACEnemyAbility_PressureCounter::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (InvincibilityEffectHandle.IsValid())
	{
		if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(InvincibilityEffectHandle);
		}
		InvincibilityEffectHandle.Invalidate();
	}

	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}
	MontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACEnemyAbility_PressureCounter::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACEnemyAbility_PressureCounter::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UACEnemyAbility_PressureCounter::OnHitTarget(FGameplayEventData Payload)
{
	const AActor* HitActor = Payload.Target.Get();
	if (!HitActor || !DamageEffect)
	{
		return;
	}

	UEnemyCombatComponent* CombatComponent = GetEnemyCombatComponentFromActorInfo();
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!CombatComponent || !ASC)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_BaseDamage, CombatComponent->GetCurrentWeaponBaseDamage());

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(HitActor)))
	{
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void UACEnemyAbility_PressureCounter::ApplyInvincibilityEffect()
{
	if (!InvincibilityEffect)
	{
		return;
	}

	if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		InvincibilityEffectHandle = ASC->ApplyGameplayEffectToSelf(
			InvincibilityEffect->GetDefaultObject<UGameplayEffect>(),
			1.0f,
			ASC->MakeEffectContext()
			);
	}
}