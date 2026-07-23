// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Block.h"
#include "ACGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
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

	// 저스트 가드(패링) 성공 대기 — 패링 판정 창(Shared.Status.Parry)은 BlockMontage의 ANS_AddGameplayTag가 관리하며,
	// 창 안에 피격되면 ACCalculation_DamageTaken이 Enemy.Event.ParrySuccess를 발송한다. 이 이벤트를 받아 카운터를 실행한다.
	// 패링 창 노티파이가 없는 기존 보스 Block은 이 이벤트가 오지 않아 리스너가 발화하지 않는다(무영향).
	ParrySuccessTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Enemy_Event_ParrySuccess, nullptr, false, true);
	ParrySuccessTask->EventReceived.AddDynamic(this, &ThisClass::OnParrySuccessEventReceived);
	ParrySuccessTask->ReadyForActivation();
}

void UACEnemyAbility_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 저스트 가드(패링) 관련 정리. Shared.Status.Parry 태그는 BlockMontage의 ANS_AddGameplayTag가 소유·해제하므로
	// 이 어빌리티에서는 건드리지 않는다. 기존 보스 Block은 태스크/핸들이 비어 있어 아래는 전부 no-op이다.
	if (ParrySuccessTask && ParrySuccessTask->IsActive())
	{
		ParrySuccessTask->EndTask();
	}
	ParrySuccessTask = nullptr;

	if (ParryCounterAttackEndedHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->OnAbilityEnded.Remove(ParryCounterAttackEndedHandle);
		}
		ParryCounterAttackEndedHandle.Reset();
	}
	ParryCounterAttackSpecHandle = FGameplayAbilitySpecHandle();

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

void UACEnemyAbility_Block::OnParrySuccessEventReceived(FGameplayEventData Payload)
{
	// 패링 판정 창 안에서 피격되어 ACCalculation_DamageTaken이 보낸 성공 이벤트.
	// 데미지 0/체간 역공/Stagger는 데미지 계산이 이미 처리했으므로, 여기서는 연출 큐와 카운터 공격만 담당한다.
	ExecuteSuccessfulParryCue(Payload);
	TryActivateParryCounterAttack();
}

void UACEnemyAbility_Block::ExecuteSuccessfulParryCue(const FGameplayEventData& Payload)
{
	if (!SuccessfulParryCueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.SourceObject = GetAvatarActorFromActorInfo();
	CueParams.Instigator = const_cast<AActor*>(Payload.Instigator.Get());

	if (const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		CueParams.TargetAttachComponent = Character->GetMesh();
	}

	K2_ExecuteGameplayCueWithParams(SuccessfulParryCueTag, CueParams);
}

bool UACEnemyAbility_Block::TryActivateParryCounterAttack()
{
	if (!ParryCounterAttackAbilityTag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	// 종료 감지에 SpecHandle이 필요하므로 TryActivateAbilitiesByTag 대신 Spec을 직접 찾아 활성화한다.
	FGameplayAbilitySpecHandle FoundHandle;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(ParryCounterAttackAbilityTag))
		{
			FoundHandle = Spec.Handle;
			break;
		}
	}

	if (!FoundHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UACEnemyAbility_Block: 패링 카운터 공격 Ability를 찾지 못했습니다 (Tag: %s) — StartupData 등록 여부를 확인하세요"), *ParryCounterAttackAbilityTag.ToString());
		return false;
	}

	// 카운터가 활성화 직후 동기적으로 끝나는 케이스도 놓치지 않도록, 활성화 전에 추적 정보를 먼저 세팅한다.
	ParryCounterAttackSpecHandle = FoundHandle;
	ParryCounterAttackEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnParryCounterAttackEnded);

	if (!ASC->TryActivateAbility(FoundHandle))
	{
		ASC->OnAbilityEnded.Remove(ParryCounterAttackEndedHandle);
		ParryCounterAttackEndedHandle.Reset();
		ParryCounterAttackSpecHandle = FGameplayAbilitySpecHandle();
		UE_LOG(LogTemp, Warning, TEXT("UACEnemyAbility_Block: 패링 카운터 공격 Ability 활성화 실패 (Tag: %s)"), *ParryCounterAttackAbilityTag.ToString());
		return false;
	}

	return true;
}

void UACEnemyAbility_Block::OnParryCounterAttackEnded(const FAbilityEndedData& EndedData)
{
	// ASC의 모든 어빌리티 종료가 이 콜백으로 들어오므로, 추적 중인 카운터 공격만 골라낸다.
	if (EndedData.AbilitySpecHandle != ParryCounterAttackSpecHandle)
	{
		return;
	}

	// 카운터 공격이 끝났으므로 Block 어빌리티를 종료한다 — 델리게이트/핸들 정리는 EndAbility가 담당한다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
