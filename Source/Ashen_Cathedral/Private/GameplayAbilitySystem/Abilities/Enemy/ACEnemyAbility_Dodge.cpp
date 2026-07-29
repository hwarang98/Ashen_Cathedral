// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Dodge.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Enemy/ACEnemyCharacter.h"

UACEnemyAbility_Dodge::UACEnemyAbility_Dodge()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Enemy_Ability_Dodge);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_Status_Dodging);

	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_Dodging);
	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_PressureCountering);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);

	// BT가 Enemy.Event.Dodge 이벤트를 전송하면 이 어빌리티가 활성화됨
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ACGameplayTags::Enemy_Event_Dodge;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACEnemyAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 압박 반응으로 선택되어 실제로 시작됐을 수 있음 — Detection이 다시 감지할 수 있도록 요청 태그를 즉시 해제한다
	// (압박과 무관한 일반 회피에서도 호출되지만, 이미 요청이 없으면 아무 동작도 하지 않아 부작용이 없다)
	UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(GetAvatarActorFromActorInfo(), ACGameplayTags::Enemy_State_PressureReady);

	if (!GetACEnemyFromActorInfo())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// SendGameplayEvent의 EventMagnitude에 담아 보낸 방향을 수신
	const float DirValue = TriggerEventData ? TriggerEventData->EventMagnitude : 1.0f;
	const EACDodgeDirection Direction = ParseDirection(DirValue);

	UAnimMontage* MontageToPlay = SelectMontage(Direction);
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 회피 무적 GE 적용 (설정된 경우에만). 비어 있으면 스킵되어 기존 동작과 완전히 동일하다.
	if (InvincibilityEffect)
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(InvincibilityEffect);
		InvincibilityEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}

	PlayDodgeMontage(MontageToPlay);
}

void UACEnemyAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 회피가 어떤 경로로 끝나든 무적 GE를 제거한다. 무적 GE를 안 쓴 경우 핸들이 무효라 no-op이므로 기존 동작에 영향 없다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveGameplayEffect(InvincibilityEffectHandle);
	}
	InvincibilityEffectHandle.Invalidate();

	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}
	MontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACEnemyAbility_Dodge::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACEnemyAbility_Dodge::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

EACDodgeDirection UACEnemyAbility_Dodge::ParseDirection(float EventMagnitude)
{
	switch (FMath::RoundToInt(EventMagnitude))
	{
		case 0:
			return EACDodgeDirection::Forward;

		case 1:
			return EACDodgeDirection::Backward;

		case 2:
			return EACDodgeDirection::Left;

		case 3:
			return EACDodgeDirection::Right;

		default:
			return EACDodgeDirection::Backward;
	}
}

UAnimMontage* UACEnemyAbility_Dodge::SelectMontage(EACDodgeDirection Direction) const
{
	switch (Direction)
	{
		case EACDodgeDirection::Forward:
			return ForwardDodgeMontage;

		case EACDodgeDirection::Backward:
			return BackDodgeMontage;

		case EACDodgeDirection::Left:
			return LeftDodgeMontage;

		case EACDodgeDirection::Right:
			return RightDodgeMontage;

		default:
			return BackDodgeMontage;
	}
}

void UACEnemyAbility_Dodge::PlayDodgeMontage(UAnimMontage* Montage)
{
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.0f, NAME_None, false);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}