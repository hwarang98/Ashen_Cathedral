// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Parry.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACEnemyAbility_Parry::UACEnemyAbility_Parry()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Enemy_Ability_Parry);
	SetAssetTags(TagsToAdd);

	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_PostureBroken);
	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_Attacking);

	// Parry 시도 확률(BTDecorator_ComputeChance)과 쿨다운(ACBTDecorator_RandomCooldown)은 BT가 전담한다.
	// 이 어빌리티는 활성화되면 항상 정상적으로 Startup -> ParryWindow를 진행한다.

	// Player Parry와 동일한 성공 큐를 재사용한다 — 에디터에서 Enemy 전용 큐로 교체 가능
	SuccessfulParryCueTag = ACGameplayTags::GameplayCue_FX_Parry;

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACEnemyAbility_Parry::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayCosmeticMontage(ParryMontage);

	StartupDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, StartupDuration);
	StartupDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnStartupDelayFinished);
	StartupDelayTask->ReadyForActivation();
}

void UACEnemyAbility_Parry::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 하드 캔슬 등 어떤 경로로 끝나든 ParryWindow 태그가 남아있지 않도록 보장한다.
	// ExitParryWindow에서 이미 제거된 경우(정상 종료)에는 태그가 없으므로, 존재할 때만 제거해
	// "태그가 컨테이너에 없는데 제거를 시도했다"는 경고가 뜨지 않게 한다.
	UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(GetAvatarActorFromActorInfo(), ACGameplayTags::Shared_Status_Parry);

	// 카운터 공격 종료 감지 델리게이트 정리 — 외부 취소를 포함한 모든 종료 경로에서 바인딩이 남지 않게 한다.
	// 진행 중인 카운터 Ability 자체는 건드리지 않는다 (Parry 종료가 카운터를 취소하면 안 됨).
	if (CounterAttackEndedDelegateHandle.IsValid())
	{
		if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
		{
			ASC->OnAbilityEnded.Remove(CounterAttackEndedDelegateHandle);
		}
		CounterAttackEndedDelegateHandle.Reset();
	}
	CounterAttackSpecHandle = FGameplayAbilitySpecHandle();

	if (StartupDelayTask && StartupDelayTask->IsActive())
	{
		StartupDelayTask->EndTask();
	}
	StartupDelayTask = nullptr;

	if (WindowTimeoutTask && WindowTimeoutTask->IsActive())
	{
		WindowTimeoutTask->EndTask();
	}
	WindowTimeoutTask = nullptr;

	if (ParrySuccessTask && ParrySuccessTask->IsActive())
	{
		ParrySuccessTask->EndTask();
	}
	ParrySuccessTask = nullptr;

	if (RecoveryDelayTask && RecoveryDelayTask->IsActive())
	{
		RecoveryDelayTask->EndTask();
	}
	RecoveryDelayTask = nullptr;

	if (MontageTask && MontageTask->IsActive())
	{
		MontageTask->EndTask();
	}
	MontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACEnemyAbility_Parry::OnStartupDelayFinished()
{
	EnterParryWindow();
}

void UACEnemyAbility_Parry::EnterParryWindow()
{
	if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(ACGameplayTags::Shared_Status_Parry);
	}

	// 타임아웃과 성공 이벤트를 동시에 대기시켜, 먼저 도착하는 쪽으로 판정을 분기한다.
	WindowTimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, ParryWindowDuration);
	WindowTimeoutTask->OnFinish.AddDynamic(this, &ThisClass::OnParryWindowTimeout);
	WindowTimeoutTask->ReadyForActivation();

	ParrySuccessTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Enemy_Event_ParrySuccess, nullptr, true);
	ParrySuccessTask->EventReceived.AddDynamic(this, &ThisClass::OnParrySuccessEventReceived);
	ParrySuccessTask->ReadyForActivation();
}

void UACEnemyAbility_Parry::OnParryWindowTimeout()
{
	ExitParryWindow(false);
}

void UACEnemyAbility_Parry::OnParrySuccessEventReceived(FGameplayEventData Payload)
{
	ExecuteSuccessfulParryCue(Payload);
	ExitParryWindow(true);
}

void UACEnemyAbility_Parry::ExitParryWindow(bool bSucceeded)
{
	UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(GetAvatarActorFromActorInfo(), ACGameplayTags::Shared_Status_Parry);

	// 레이스에서 진 태스크를 정리한다 (이긴 쪽에서 이 함수가 호출되므로 둘 다 안전하게 정리 가능)
	if (WindowTimeoutTask && WindowTimeoutTask->IsActive())
	{
		WindowTimeoutTask->EndTask();
	}
	WindowTimeoutTask = nullptr;

	if (ParrySuccessTask && ParrySuccessTask->IsActive())
	{
		ParrySuccessTask->EndTask();
	}
	ParrySuccessTask = nullptr;

	if (bSucceeded)
	{
		// 카운터 공격은 연출 몽타주 직접 재생 대신, 실제 공격 판정/데미지를 갖춘 별도 Ability를 태그로 실행한다.
		// Shared.Status.Parry는 위에서 이미 제거됐으므로 카운터 Ability가 Parry 상태에 막히지 않는다.
		// 카운터가 실제로 활성화되면 그 종료 시점까지 이 Parry 어빌리티를 살려둬서,
		// BT의 ActivateAbilityByTagAndWait(Enemy.Ability.Parry)가 카운터 도중 완료되어
		// 다른 브랜치(Block 등)로 넘어가는 것을 막는다. 종료는 OnCounterAttackAbilityEnded가 담당한다.
		if (TryActivateCounterAttackAbility())
		{
			return;
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (FailureRecoveryDuration > 0.f)
	{
		RecoveryDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FailureRecoveryDuration);
		RecoveryDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnFailureRecoveryFinished);
		RecoveryDelayTask->ReadyForActivation();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACEnemyAbility_Parry::OnFailureRecoveryFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACEnemyAbility_Parry::PlayCosmeticMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.0f, NAME_None, false);
	MontageTask->ReadyForActivation();
}

FGameplayCueParameters UACEnemyAbility_Parry::MakeParryGameplayCueParams(const FGameplayEventData& Payload) const
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

void UACEnemyAbility_Parry::ExecuteSuccessfulParryCue(const FGameplayEventData& Payload)
{
	if (!SuccessfulParryCueTag.IsValid())
	{
		return;
	}

	K2_ExecuteGameplayCueWithParams(SuccessfulParryCueTag, MakeParryGameplayCueParams(Payload));
}

bool UACEnemyAbility_Parry::TryActivateCounterAttackAbility()
{
	if (!CounterAttackAbilityTag.IsValid())
	{
		return false;
	}

	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	// 종료 감지에 SpecHandle이 필요하므로 TryActivateAbilitiesByTag 대신 Spec을 직접 찾아 활성화한다.
	FGameplayAbilitySpecHandle FoundHandle;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(CounterAttackAbilityTag))
		{
			FoundHandle = Spec.Handle;
			break;
		}
	}

	if (!FoundHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UACEnemyAbility_Parry: 카운터 공격 Ability를 찾지 못했습니다 (Tag: %s) — StartupData 등록 여부를 확인하세요"), *CounterAttackAbilityTag.ToString());
		return false;
	}

	// 카운터가 활성화 직후 동기적으로 끝나는 케이스(Commit 실패 등)도 놓치지 않도록,
	// 활성화 전에 SpecHandle 기록과 종료 델리게이트 바인딩을 먼저 해둔다.
	CounterAttackSpecHandle = FoundHandle;
	CounterAttackEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnCounterAttackAbilityEnded);

	if (!ASC->TryActivateAbility(FoundHandle))
	{
		// 어빌리티 미부여/조건 불충족 등으로 실패해도 Parry 성공 흐름은 계속 진행된다 (호출부가 EndAbility 처리).
		ASC->OnAbilityEnded.Remove(CounterAttackEndedDelegateHandle);
		CounterAttackEndedDelegateHandle.Reset();
		CounterAttackSpecHandle = FGameplayAbilitySpecHandle();
		UE_LOG(LogTemp, Warning, TEXT("UACEnemyAbility_Parry: 카운터 공격 Ability 활성화 실패 (Tag: %s)"), *CounterAttackAbilityTag.ToString());
		return false;
	}

	return true;
}

void UACEnemyAbility_Parry::OnCounterAttackAbilityEnded(const FAbilityEndedData& EndedData)
{
	// ASC의 모든 어빌리티 종료가 이 콜백으로 들어오므로, 추적 중인 카운터 공격만 골라낸다.
	if (EndedData.AbilitySpecHandle != CounterAttackSpecHandle)
	{
		return;
	}

	// 카운터 공격이 끝났으므로 Parry 어빌리티를 종료한다 — 델리게이트/핸들 정리는 EndAbility가 담당한다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
