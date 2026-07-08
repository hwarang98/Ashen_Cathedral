// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Dodge.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
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

	// BT가 SendGameplayEvent의 EventMagnitude에 담아 보낸 방향을 수신
	const float DirValue = TriggerEventData ? TriggerEventData->EventMagnitude : 1.0f;
	const EDodgeDirection Direction = ParseDirection(DirValue);

	UAnimMontage* MontageToPlay = SelectMontage(Direction);
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayDodgeMontage(MontageToPlay);
}

void UACEnemyAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
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

UACEnemyAbility_Dodge::EDodgeDirection UACEnemyAbility_Dodge::ParseDirection(float EventMagnitude)
{
	switch (FMath::RoundToInt(EventMagnitude))
	{
		case 0:
			return EDodgeDirection::Forward;

		case 1:
			return EDodgeDirection::Backward;

		case 2:
			return EDodgeDirection::Left;

		case 3:
			return EDodgeDirection::Right;

		default:
			return EDodgeDirection::Backward;
	}
}

UAnimMontage* UACEnemyAbility_Dodge::SelectMontage(EDodgeDirection Direction) const
{
	switch (Direction)
	{
		case EDodgeDirection::Forward:
			return ForwardDodgeMontage;

		case EDodgeDirection::Backward:
			return BackDodgeMontage;

		case EDodgeDirection::Left:
			return LeftDodgeMontage;

		case EDodgeDirection::Right:
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