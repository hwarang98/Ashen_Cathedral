// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Block.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"

UACEnemyAbility_Block::UACEnemyAbility_Block()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Enemy_Ability_Block);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_Status_Blocking);

	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_Attacking);

	// Player Block과 동일한 성공 큐를 Enemy ASC에서 실행한다 — 에디터에서 Enemy 전용 큐로 교체 가능
	SuccessfulBlockCueTag = ACGameplayTags::GameplayCue_FX_SuccessfulBlock;
}

void UACEnemyAbility_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	// Block 성공 이벤트 대기 — TryTriggerSuccessfulBlockEvent가 방어자(HitActor=이 Enemy)에게 보내는
	// Player.Event.SuccessfulBlock을 그대로 수신한다 (UACPlayerAbility_Block과 동일 패턴, Player 쪽 발송 로직 변경 없음).
	WaitBlockEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Player_Event_SuccessfulBlock, nullptr, false, true);
	WaitBlockEventTask->EventReceived.AddDynamic(this, &ThisClass::OnSuccessfulBlockEventReceived);
	WaitBlockEventTask->ReadyForActivation();

	// Block 자세 몽타주 재생 — 이 몽타주의 자연 종료가 곧 "Block 유지 시간 종료"다.
	PlayHoldMontage();
}

void UACEnemyAbility_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}
	MontageTask = nullptr;

	// 부모 EndAbility에서 MoveSpeedEffectHandle GE 제거
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACEnemyAbility_Block::PlayHoldMontage()
{
	// 의도적 몽타주 전환(BlockHit 복귀 등)이 이전 태스크의 OnInterrupted를 거쳐 EndAbility로 이어지지 않도록
	// 기존 태스크를 콜백 없이 조용히 종료한다.
	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, BlockMontage, 1.0f, NAME_None, false);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

void UACEnemyAbility_Block::OnSuccessfulBlockEventReceived(FGameplayEventData Payload)
{
	// Block 성공 큐를 Enemy ASC에서 실행 (Player의 SuccessfulBlock 큐 흐름과는 독립)
	if (SuccessfulBlockCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.SourceObject = GetAvatarActorFromActorInfo();
		CueParams.Instigator = const_cast<AActor*>(Payload.Instigator.Get());

		if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			CueParams.TargetAttachComponent = Character->GetMesh();
		}

		K2_ExecuteGameplayCueWithParams(SuccessfulBlockCueTag, CueParams);
	}

	// 반응 몽타주가 없으면 자세 몽타주를 그대로 유지한다.
	if (!BlockHitMontage)
	{
		return;
	}

	// 의도적 전환: 자세 몽타주 태스크를 조용히 종료해 OnInterrupted → EndAbility가 발생하지 않게 한다.
	// Blocking 태그와 어빌리티는 계속 유지된다.
	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, BlockHitMontage, 1.0f, NAME_None, false);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnBlockHitMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnBlockHitMontageFinished);
	// 외부 강제 중단(그로기/사망 몽타주 오버라이드)만 여기로 온다 — 진짜 종료 조건.
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

void UACEnemyAbility_Block::OnBlockHitMontageFinished()
{
	// 움찔 반응이 끝나면 다시 Block 자세로 복귀한다 — 어빌리티와 Blocking 태그는 이 사이에도 계속 유지 중이다.
	PlayHoldMontage();
}

void UACEnemyAbility_Block::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACEnemyAbility_Block::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
