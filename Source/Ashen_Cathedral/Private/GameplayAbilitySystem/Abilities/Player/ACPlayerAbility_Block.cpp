// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Block.h"
#include "ACGameplayTags.h"
#include "ACFunctionLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Character/ACCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Kismet/KismetMathLibrary.h"

UACPlayerAbility_Block::UACPlayerAbility_Block()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Block);
	SetAssetTags(TagsToAdd);
}

void UACPlayerAbility_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!BlockMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 이동 속도 제한 GE 적용
	if (MoveSpeedEffect)
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(MoveSpeedEffect);
		MoveSpeedEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}

	// 블록 몽타주 재생
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		BlockMontage,
		1.0f,
		NAME_None,
		false
		);

	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnBlendedIn.AddDynamic(this, &ThisClass::OnMontageBlendedIn);
	MontageTask->ReadyForActivation();

	// Player.Event.SuccessfulBlock 이벤트 대기
	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ACGameplayTags::Player_Event_SuccessfulBlock,
		nullptr,
		false,
		true
		);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnSuccessfulBlockEventReceived);
	WaitEventTask->ReadyForActivation();
}

void UACPlayerAbility_Block::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UACPlayerAbility_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 카운터어택 윈도우 태그 제거 및 타이머 정리
	if (AACCharacterBase* Character = GetACCharacterFromActorInfo())
	{
		UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(Character, ACGameplayTags::Shared_Status_CanCounterAttack);
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CounterAttackTimerHandle);
	}

	// 지속형 블록 GameplayCue 제거
	K2_RemoveGameplayCue(ACGameplayTags::GameplayCue_FX_Block);

	// 부모 EndAbility에서 MoveSpeedEffectHandle GE 제거
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACPlayerAbility_Block::OnMontageCompleted()
{
}

void UACPlayerAbility_Block::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UACPlayerAbility_Block::OnMontageBlendedIn()
{
	// 지속형 블록 GameplayCue 추가
	const FGameplayCueParameters CueParams = MakeBlockGameplayCueParams(FGameplayEventData());
	K2_AddGameplayCueWithParams(ACGameplayTags::GameplayCue_FX_Block, CueParams);
}

void UACPlayerAbility_Block::OnSuccessfulBlockEventReceived(FGameplayEventData Payload)
{
	CachedPayload = Payload;

	RotateActorToTargetFromEventData(Payload);

	const AACCharacterBase* Character = GetACCharacterFromActorInfo();
	const bool bIsParry = Character &&
		UACFunctionLibrary::NativeDoesActorHaveTag(const_cast<AACCharacterBase*>(Character), ACGameplayTags::Shared_Status_Parry);

	if (bIsParry)
	{
		ExecuteParryCue(Payload);
		StartCounterAttackWindow();
	}
	else
	{
		ExecuteSuccessfulBlockCue(Payload);
	}

	// 블록 히트 시 공격자 반대 방향으로 밀려나는 RootMotion 적용
	if (Character)
	{
		const FVector PushbackDirection = -Character->GetActorForwardVector();

		UAbilityTask_ApplyRootMotionConstantForce* RootMotionTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this,
			NAME_None,
			PushbackDirection,
			HitPushbackStrength,
			HitPushbackDuration,
			false,
			nullptr,
			ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
			FVector::ZeroVector,
			0.f,
			false
			);
		RootMotionTask->ReadyForActivation();
	}
}

void UACPlayerAbility_Block::StartCounterAttackWindow()
{
	AACCharacterBase* Character = GetACCharacterFromActorInfo();
	if (!Character)
	{
		return;
	}

	UACFunctionLibrary::AddGameplayTagToActorIfNone(Character, ACGameplayTags::Shared_Status_CanCounterAttack);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CounterAttackTimerHandle,
			this,
			&ThisClass::ResetCounterAttackWindow,
			CounterAttackWindowDuration,
			false
			);
	}
}

void UACPlayerAbility_Block::ResetCounterAttackWindow()
{
	if (AACCharacterBase* Character = GetACCharacterFromActorInfo())
	{
		UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(Character, ACGameplayTags::Shared_Status_CanCounterAttack);
	}
}

void UACPlayerAbility_Block::RotateActorToTargetFromEventData(const FGameplayEventData& Payload) const
{
	AACCharacterBase* Character = GetACCharacterFromActorInfo();
	const AActor* Instigator = Payload.Instigator.Get();
	if (!Character || !Instigator)
	{
		return;
	}

	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(
		Character->GetActorLocation(),
		Instigator->GetActorLocation()
		);
	LookAtRot.Pitch = 0.f;
	LookAtRot.Roll = 0.f;

	Character->SetActorRotation(LookAtRot);
}

FGameplayCueParameters UACPlayerAbility_Block::MakeBlockGameplayCueParams(const FGameplayEventData& Payload) const
{
	FGameplayCueParameters Params;
	Params.SourceObject = GetAvatarActorFromActorInfo();
	Params.Instigator = const_cast<AActor*>(Payload.Instigator.Get());

	if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Params.TargetAttachComponent = Character->GetMesh();
	}

	return Params;
}

void UACPlayerAbility_Block::ExecuteSuccessfulBlockCue(const FGameplayEventData& Payload)
{
	K2_ExecuteGameplayCueWithParams(ACGameplayTags::GameplayCue_FX_SuccessfulBlock, MakeBlockGameplayCueParams(Payload));
}

void UACPlayerAbility_Block::ExecuteParryCue(const FGameplayEventData& Payload)
{
	K2_ExecuteGameplayCueWithParams(ACGameplayTags::GameplayCue_FX_Parry, MakeBlockGameplayCueParams(Payload)
		);
}