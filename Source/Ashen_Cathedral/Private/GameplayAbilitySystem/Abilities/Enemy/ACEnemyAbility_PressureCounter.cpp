// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_PressureCounter.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Components/Combat/AOEDamageComponent.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Items/Weapon/ACWeaponBase.h"

UACEnemyAbility_PressureCounter::UACEnemyAbility_PressureCounter()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Enemy_Ability_Pressure_Counter);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor);
	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_Invincible);
	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_Status_PressureCountering);

	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_PostureBroken);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Executed);
	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_Dodging);

	// 압박 반격 발동 시 진행 중인 공격 어빌리티를 강제로 취소해 몽타주가 겹치지 않게 한다
	CancelAbilitiesWithTag.AddTag(ACGameplayTags::Enemy_Ability_Melee);

	// 기본값: Parry 가능 / Block 불가 — 그냥 Block으로는 뚫리고, 정확한 타이밍의 Parry만 성공해야 한다.
	PressureCounterDefenseTags.AddTag(ACGameplayTags::Shared_Attack_Parryable);
	PressureCounterDefenseTags.AddTag(ACGameplayTags::Shared_Attack_Unblockable);

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

	// 몽타주에 AOE 노티파이(AN_AOEInstant / ANS_AOESustained)가 없으면 이벤트가 오지 않으므로
	// 별도 활성화 옵션 없이 항상 대기한다 — MeleeHit 대기 태스크와 동일한 패턴.
	UAbilityTask_WaitGameplayEvent* WaitInstantAOETask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Shared_Event_AOE_Instant);
	WaitInstantAOETask->EventReceived.AddDynamic(this, &ThisClass::OnInstantAOEEventReceived);
	WaitInstantAOETask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitSustainedStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Shared_Event_AOE_Sustained_Start);
	WaitSustainedStartTask->EventReceived.AddDynamic(this, &ThisClass::OnSustainedAOEStartReceived);
	WaitSustainedStartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitSustainedEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ACGameplayTags::Shared_Event_AOE_Sustained_End);
	WaitSustainedEndTask->EventReceived.AddDynamic(this, &ThisClass::OnSustainedAOEEndReceived);
	WaitSustainedEndTask->ReadyForActivation();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CounterMontage, 1.0f, NAME_None, false);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

void UACEnemyAbility_PressureCounter::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 지속형 AOE가 진행 중이었다면 컴포넌트의 타이머와 중복 히트 목록을 정리한다.
	if (AACEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo())
	{
		if (UAOEDamageComponent* AOEComponent = EnemyCharacter->FindComponentByClass<UAOEDamageComponent>())
		{
			AOEComponent->StopSustainedAOE();
		}
	}

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
	if (!HitActor)
	{
		return;
	}

	UEnemyCombatComponent* CombatComponent = GetEnemyCombatComponentFromActorInfo();
	if (!CombatComponent)
	{
		return;
	}

	// AOE 경로(OnInstantAOEEventReceived/OnSustainedAOEStartReceived)와 동일하게, 유효한 Parry/Block이면
	// 데미지 적용 전에 대상에게 GameplayCue를 발동시킨다.
	UACFunctionLibrary::TryTriggerSuccessfulBlockEvent(GetAvatarActorFromActorInfo(), const_cast<AActor*>(HitActor), PressureCounterDefenseTags);

	ApplyDamageEffectSpecToTarget(HitActor, CombatComponent->GetCurrentWeaponBaseDamage(), 0.f);
}

void UACEnemyAbility_PressureCounter::OnInstantAOEEventReceived(FGameplayEventData Payload)
{
	// 서버 권한에서만 판정한다 — 클라이언트에서의 중복 판정/데미지 적용을 방지한다.
	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AACEnemyCharacter* OwnerCharacter = GetEnemyCharacterFromActorInfo();
	UEnemyCombatComponent* CombatComponent = GetEnemyCombatComponentFromActorInfo();
	UAOEDamageComponent* AOEComponent = GetOrCreateAOEDamageComponent();
	if (!OwnerCharacter || !CombatComponent || !AOEComponent || !DamageEffect)
	{
		return;
	}

	const float BaseDamage = CombatComponent->GetCurrentWeaponBaseDamage() * AOEBaseDamageMultiplier;
	const float PostureDamage = AOEPostureDamage;

	AOEComponent->TriggerInstantAOE(InstantAOERadius, InstantAOEForwardOffset, bDebugDrawAOE,
		[this, OwnerCharacter, BaseDamage, PostureDamage](AActor* TargetActor)
		{
			// 무기 콜리전 근접 공격과 동일하게, 유효한 Parry/Block이면 대상에게 GameplayCue를 발동시킨다.
			UACFunctionLibrary::TryTriggerSuccessfulBlockEvent(OwnerCharacter, TargetActor, PressureCounterDefenseTags);
			ApplyDamageEffectSpecToTarget(TargetActor, BaseDamage, PostureDamage);
		});
}

void UACEnemyAbility_PressureCounter::OnSustainedAOEStartReceived(FGameplayEventData Payload)
{
	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AACEnemyCharacter* OwnerCharacter = GetEnemyCharacterFromActorInfo();
	UEnemyCombatComponent* CombatComponent = GetEnemyCombatComponentFromActorInfo();
	UAOEDamageComponent* AOEComponent = GetOrCreateAOEDamageComponent();
	if (!OwnerCharacter || !CombatComponent || !AOEComponent || !DamageEffect)
	{
		return;
	}

	const float BaseDamage = CombatComponent->GetCurrentWeaponBaseDamage() * AOEBaseDamageMultiplier;
	const float PostureDamage = AOEPostureDamage;

	AOEComponent->StartSustainedAOE(SustainedAOERadius, SustainedAOEForwardOffset, SustainedAOEDamageInterval, bDebugDrawAOE,
		[this, OwnerCharacter, BaseDamage, PostureDamage](AActor* TargetActor)
		{
			// 무기 콜리전 근접 공격과 동일하게, 유효한 Parry/Block이면 대상에게 GameplayCue를 발동시킨다.
			UACFunctionLibrary::TryTriggerSuccessfulBlockEvent(OwnerCharacter, TargetActor, PressureCounterDefenseTags);
			ApplyDamageEffectSpecToTarget(TargetActor, BaseDamage, PostureDamage);
		});
}

void UACEnemyAbility_PressureCounter::OnSustainedAOEEndReceived(FGameplayEventData Payload)
{
	if (AACEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo())
	{
		if (UAOEDamageComponent* AOEComponent = EnemyCharacter->FindComponentByClass<UAOEDamageComponent>())
		{
			AOEComponent->StopSustainedAOE();
		}
	}
}

UAOEDamageComponent* UACEnemyAbility_PressureCounter::GetOrCreateAOEDamageComponent() const
{
	AACEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo();
	if (!EnemyCharacter)
	{
		return nullptr;
	}

	if (UAOEDamageComponent* Existing = EnemyCharacter->FindComponentByClass<UAOEDamageComponent>())
	{
		return Existing;
	}

	UAOEDamageComponent* NewComponent = NewObject<UAOEDamageComponent>(EnemyCharacter);
	NewComponent->RegisterComponent();
	return NewComponent;
}

bool UACEnemyAbility_PressureCounter::ApplyDamageEffectSpecToTarget(const AActor* TargetActor, float BaseDamage, float PostureDamage)
{
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!ASC || !TargetActor || !DamageEffect)
	{
		return false;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_BaseDamage, BaseDamage);

	if (PostureDamage > 0.f)
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_PostureDamage, PostureDamage);
	}

	// PressureCounterDefenseTags를 Spec의 DynamicAssetTags에 실어, ACCalculation_DamageTaken/IsSuccessfulParry/
	// IsSuccessfulBlock이 이 반격의 방어 가능 속성을 실제 Hit 시점에 판정할 수 있게 한다.
	if (!PressureCounterDefenseTags.IsEmpty() && SpecHandle.Data.IsValid())
	{
		SpecHandle.Data->AppendDynamicAssetTags(PressureCounterDefenseTags);
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(TargetActor));
	if (!TargetASC)
	{
		return false;
	}

	ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	PlayHitGameplayCue(TargetActor);
	return true;
}

void UACEnemyAbility_PressureCounter::PlayHitGameplayCue(const AActor* HitActor) const
{
	AACEnemyCharacter* OwnerCharacter = GetEnemyCharacterFromActorInfo();
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!OwnerCharacter || !ASC || !HitActor)
	{
		return;
	}

	// 실제 Parry/Block 성공(ACCalculation_DamageTaken과 동일 기준: PressureCounterDefenseTags)일 때만 대상 쪽에서
	// 별도의 Block/Parry GameplayCue가 재생되므로 일반 히트 큐는 생략한다. Invincible/Dead 상태는 데미지 자체가
	// 0으로 무효화되므로("맞은 효과"가 없으므로) 마찬가지로 재생하지 않는다.
	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(HitActor));
	if (TargetASC)
	{
		const bool bParrySuccess = UACFunctionLibrary::IsSuccessfulParry(OwnerCharacter, HitActor, PressureCounterDefenseTags);
		const bool bBlockSuccess = UACFunctionLibrary::IsSuccessfulBlock(OwnerCharacter, HitActor, PressureCounterDefenseTags);

		if (bParrySuccess || bBlockSuccess
			|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible)
			|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
		{
			return;
		}
	}

	const UEnemyCombatComponent* CombatComponent = GetEnemyCombatComponentFromActorInfo();

	FGameplayCueParameters CueParams;
	CueParams.Instigator = OwnerCharacter;
	CueParams.EffectCauser = OwnerCharacter;
	CueParams.SourceObject = CombatComponent ? Cast<AACWeaponBase>(CombatComponent->GetCharacterCurrentEquippedWeapon()) : nullptr;
	CueParams.TargetAttachComponent = HitActor->GetRootComponent();
	CueParams.Location = HitActor->GetActorLocation();
	CueParams.Normal = (OwnerCharacter->GetActorLocation() - HitActor->GetActorLocation()).GetSafeNormal();

	ASC->ExecuteGameplayCue(HitGameplayCueTag, CueParams);
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