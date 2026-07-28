// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Block.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "ACFunctionLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Animation/AnimInstance.h"
#include "Character/ACCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

UACPlayerAbility_Block::UACPlayerAbility_Block()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Block);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_Blocking);

	// 가드가 무너진 경직 동안에는 다시 막을 수 없다
	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_GuardBroken);
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

	// AttributeSet이 GuardGauge 최대치 도달 시 보내는 가드 붕괴 통보를 대기한다
	WaitGuardBrokenTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ACGameplayTags::Shared_Event_GuardBrokenTriggered,
		nullptr,
		false,
		true
		);
	WaitGuardBrokenTask->EventReceived.AddDynamic(this, &ThisClass::OnGuardBrokenEventReceived);
	WaitGuardBrokenTask->ReadyForActivation();
}

void UACPlayerAbility_Block::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UACPlayerAbility_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 카운터어택 윈도우는 GameplayEffect의 Duration이 단독으로 관리한다.
	// Block 어빌리티가 끝나도(입력 릴리즈 등) 건드리지 않아야
	// 패링 성공 직후 반격을 위해 Block 입력을 떼는 순간 윈도우가 파괴되지 않는다.

	// 지속형 블록 GameplayCue 제거
	K2_RemoveGameplayCue(ACGameplayTags::GameplayCue_FX_Block);

	// 부모 EndAbility에서 MoveSpeedEffectHandle GE 제거
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACPlayerAbility_Block::OnMontageCompleted() {}

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
	const bool bIsParry = Character && UACFunctionLibrary::NativeDoesActorHaveTag(const_cast<AACCharacterBase*>(Character), ACGameplayTags::Shared_Status_Parry);

	if (bIsParry)
	{
		ExecuteParryCue(Payload);
		ApplyCounterAttackWindowEffect();
	}
	else
	{
		ExecuteSuccessfulBlockCue(Payload);

		// 패링은 완전 방어이므로 게이지 누적 대상에서 제외한다 — 순수 블록으로 막아낸 타격만 가드를 깎는다.
		// 임계값 판정은 AttributeSet이 하며, 도달 시 OnGuardBrokenEventReceived로 되돌아온다.
		ApplyGuardDamage(Payload);
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

void UACPlayerAbility_Block::ApplyGuardDamage(const FGameplayEventData& Payload)
{
	// GE가 비어 있으면 가드 게이지 기능 자체가 꺼진 것으로 본다(기존 블록 동작 유지)
	if (!GuardDamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Guard] GuardDamageEffect가 비어 있음 — GA_Player_Block에서 지정 필요"));
		return;
	}

	const bool bIsHeavyHit = GuardBreakWeightTag.IsValid() && Payload.InstigatorTags.HasTag(GuardBreakWeightTag);
	const float GuardDamage = bIsHeavyHit ? GuardBreakHeavyAmount : GuardBreakAmountPerHit;

	UE_LOG(LogTemp, Warning, TEXT("[Guard] ApplyGuardDamage 호출 — 부하=%.1f Heavy=%s"), GuardDamage, bIsHeavyHit ? TEXT("true") : TEXT("false"));

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GuardDamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Guard] SpecHandle 생성 실패"));
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_GuardDamage, GuardDamage);
	ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}

void UACPlayerAbility_Block::OnGuardBrokenEventReceived(FGameplayEventData Payload)
{
	TriggerGuardBreak();
}

void UACPlayerAbility_Block::TriggerGuardBreak()
{
	if (GuardBreakEffect)
	{
		ApplyGameplayEffectToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, GuardBreakEffect->GetDefaultObject<UGameplayEffect>(), GetAbilityLevel());
	}

	// EndAbility가 Block 몽타주 태스크를 정리하므로, 애님 인스턴스를 먼저 잡아두고 종료 후에 재생한다
	const AACCharacterBase* Character = GetACCharacterFromActorInfo();
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* MontageToPlay = GuardBreakMontage;

	// 어빌리티가 끝나면서 Blocking 태그와 이동 제한 GE가 해제되어 실제로 방어가 풀린다
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

	if (AnimInstance && MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay, 1.0f);
	}
}

void UACPlayerAbility_Block::ApplyCounterAttackWindowEffect()
{
	if (!CounterAttackWindowEffect)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CounterAttackWindowEffect);
	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
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
	K2_ExecuteGameplayCueWithParams(ACGameplayTags::GameplayCue_FX_Parry, MakeBlockGameplayCueParams(Payload));
}