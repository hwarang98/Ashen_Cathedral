// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ACGameplayTags.h"
#include "Camera/CameraShakeBase.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/AOEDamageComponent.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Items/Weapon/ACWeaponBase.h"

UACAbility_Attack::UACAbility_Attack()
{
	// 사망 상태에서는 공격 불가
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);

	// 공격 중에는 슈퍼아머 부여 (피격 시 HitReact 무시)
	ActivationOwnedTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor);

	// 콤보 카운트(CurrentComboCount)를 어빌리티 인스턴스에 유지하기 위해 InstancedPerActor 사용
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UACAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (AttackMontages.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// CommitAbility: 스태미나 소모 등 어빌리티 비용을 적용한다.
	// 비용이 부족하면 즉시 종료
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	UPawnCombatComponent* CombatComponent = OwnerCharacter ? OwnerCharacter->GetPawnCombatComponent() : nullptr;
	if (!CombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	// Shared_Status_CanCounterAttack 태그가 있으면 카운터 어택으로 처리
	// OnHitTarget에서도 동일한 판정을 써야 하므로 멤버에 캐시해둔다.
	bWasCounterAttack = ASC && ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_CanCounterAttack);

	UAnimMontage* MontageToPlay;

	if (UAnimMontage* CounterMontage = bWasCounterAttack ? SelectCounterAttackMontage() : nullptr)
	{
		// 카운터 어택: 랜덤으로 고른 전용 몽타주 재생 및 콤보 카운트 초기화
		MontageToPlay = CounterMontage;
		CurrentComboCount = 0;

		// 카운터어택 윈도우 소모 — 남은 시간 동안의 후속 공격이 계속 카운터로 처리되지 않도록
		// 윈도우를 부여한 GameplayEffect를 즉시 제거한다 (남은 지속시간과 무관하게 종료됨)
		FGameplayTagContainer TagsToRemove;
		TagsToRemove.AddTag(ACGameplayTags::Shared_Status_CanCounterAttack);
		ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
	}
	else
	{
		MontageToPlay = SelectAttackMontage();
	}

	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 무기 충돌(OnHitTargetActor)이 발생하면 PawnCombatComponent가
	// Shared_Event_MeleeHit 이벤트를 전송한다.
	// 이 Task는 해당 이벤트를 수신하여 OnHitTarget을 호출한다.
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

	// DataTable의 AttackSpeed를 몽타주 재생 속도로 사용
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		CombatComponent->GetCurrentWeaponAttackSpeed(),
		NAME_None,
		false);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 카운터 어택이 아닌 경우에만 콤보 카운트 증가
	// OnHitTarget에서 이 값을 SetByCaller로 데미지 계산기에 전달한다
	if (!bWasCounterAttack)
	{
		CurrentComboCount++;
	}
}

void UACAbility_Attack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 지속형 AOE가 진행 중이었다면 컴포넌트의 타이머와 중복 히트 목록을 정리한다.
	if (AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo())
	{
		if (UAOEDamageComponent* AOEComponent = OwnerCharacter->FindComponentByClass<UAOEDamageComponent>())
		{
			AOEComponent->StopSustainedAOE();
		}
	}

	// 피격/닷지 등 외부 요인으로 취소된 경우 콤보를 즉시 리셋
	if (bWasCancelled)
	{
		HandleComboCancelled();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UACAbility_Attack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UACAbility_Attack::HandleComboComplete() {}

UAnimMontage* UACAbility_Attack::SelectAttackMontage()
{
	if (CurrentComboCount >= AttackMontages.Num())
	{
		CurrentComboCount = 0;
	}
	return AttackMontages[CurrentComboCount];
}

UAnimMontage* UACAbility_Attack::SelectCounterAttackMontage() const
{
	if (CounterAttackMontages.IsEmpty())
	{
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, CounterAttackMontages.Num() - 1);
	return CounterAttackMontages[RandomIndex];
}

void UACAbility_Attack::HandleComboCancelled()
{
	ResetComboCount();
}

void UACAbility_Attack::ResetComboCount()
{
	CurrentComboCount = 0;
}

void UACAbility_Attack::RequestSoftMontageCancel()
{
	bSoftCancelRequested = true;
}

void UACAbility_Attack::OnMontageEnded()
{
	if (!IsActive())
	{
		return;
	}

	// EndAbility를 먼저 호출해 task cleanup을 완료한다.
	// HandleComboComplete는 cleanup 이후에 호출해야 재진입 ClearTimer 문제가 발생하지 않는다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	HandleComboComplete();
}

void UACAbility_Attack::OnMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}

	// 이동/회피로 인한 조기 캔슬은 히트리액트 같은 강제 캔슬과 구분해
	// 콤보를 즉시 리셋하지 않고 자연 완료(OnMontageEnded)와 동일하게 처리한다.
	if (bSoftCancelRequested)
	{
		bSoftCancelRequested = false;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		HandleComboComplete();
		return;
	}

	// bWasCancelled=true → EndAbility 내부에서 HandleComboCancelled 호출
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

FGameplayEffectSpecHandle UACAbility_Attack::CreateDamageEffectSpec(float BaseDamage, float GroggyDamage)
{
	if (!DamageEffect)
	{
		return FGameplayEffectSpecHandle();
	}

	// Effect Spec 생성: DamageEffect(GE)의 껍데기를 만든다.
	// 이 시점에는 SetByCaller 값이 비어있다.
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return SpecHandle;
	}

	// SetByCaller: 태그를 키로 동적 값을 Spec에 주입한다.
	// ACCalculation_DamageTaken에서 이 태그들을 키로 값을 꺼내 최종 데미지를 계산한다.
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_BaseDamage, BaseDamage);

	if (GroggyDamage > 0.f)
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_GroggyDamage, GroggyDamage);
	}

	return SpecHandle;
}

bool UACAbility_Attack::ApplyDamageEffectSpecToTarget(const FGameplayEffectSpecHandle& SpecHandle, const AActor* HitActor, float BaseDamage)
{
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!ASC || !HitActor || !SpecHandle.IsValid())
	{
		return false;
	}

	// GE 적용 직전: 서브클래스가 추가 SetByCaller 값을 동일 Spec에 주입할 수 있는 확장 포인트
	ModifyDamageSpec(SpecHandle, HitActor, BaseDamage);

	// 적용 순서: ApplyGameplayEffectSpecToTarget
	//           → ACCalculation_DamageTaken::Execute (SetByCaller 값으로 최종 데미지 계산)
	//           → ACAttributeSet::PostGameplayEffectExecute (DamageTaken → Health 차감, GroggyGauge 증가)
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(HitActor));
	if (!TargetASC)
	{
		return false;
	}

	ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

void UACAbility_Attack::PlayHitGameplayCue(const AActor* HitActor) const
{
	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!OwnerCharacter || !ASC || !HitActor)
	{
		return;
	}

	// 대상이 Block/Parry 중이면 대상 쪽에서 별도의 Block/Parry GameplayCue가 재생되므로(TryTriggerSuccessfulBlockEvent → GA_Block)
	// 일반 히트 사운드는 생략한다. Invincible/Dead 상태는 ACAttributeSet::HandleDamageAndTriggerHitReact에서
	// 데미지 자체가 0으로 무효화되므로("맞은 효과"가 없으므로) 마찬가지로 재생하지 않는다.
	// ACCalculation_DamageTaken/ACAttributeSet도 이 태그들을 기준으로 데미지를 보정·무효화하므로 판정 기준이 일치한다.
	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(HitActor));
	if (TargetASC && (TargetASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_Blocking)
		|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Parry)
		|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible)
		|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead)))
	{
		return;
	}

	const UPawnCombatComponent* CombatComponent = OwnerCharacter->GetPawnCombatComponent();

	FGameplayCueParameters CueParams;
	CueParams.Instigator = OwnerCharacter;
	CueParams.EffectCauser = OwnerCharacter;
	CueParams.SourceObject = CombatComponent ? Cast<AACWeaponBase>(CombatComponent->GetCharacterCurrentEquippedWeapon()) : nullptr; // Player/Enemy 모두 사용 가능한 PawnCombatComponent 함수 사용
	CueParams.TargetAttachComponent = HitActor->GetRootComponent();
	CueParams.Location = HitActor->GetActorLocation();
	CueParams.Normal = (OwnerCharacter->GetActorLocation() - HitActor->GetActorLocation()).GetSafeNormal();

	ASC->ExecuteGameplayCue(MeleeAttackSoundCueTag, CueParams);
}

void UACAbility_Attack::OnHitTarget(FGameplayEventData Payload)
{
	const AActor* HitActor = Payload.Target.Get();
	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();

	if (!OwnerCharacter || !HitActor || !DamageEffect)
	{
		return;
	}

	const UPawnCombatComponent* CombatComponent = OwnerCharacter->GetPawnCombatComponent();
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!CombatComponent || !ASC)
	{
		return;
	}

	// virtual dispatch를 통해 Player/Enemy 각자의 DataTable 값을 가져온다
	const float BaseDamage = CombatComponent->GetCurrentWeaponBaseDamage();
	// ActivateAbility에서 태그를 이미 소모했으므로, 여기서 태그를 다시 조회하지 않고 캐시된 값을 사용한다.
	const bool bIsCounterAttack = bWasCounterAttack;

	float GroggyDamage = 0.f;
	if (bIsCounterAttack)
	{
		GroggyDamage = CombatComponent->GetCurrentWeaponCounterAttackGroggyDamage();
	}
	else if (ComboAttackTypeTag.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_AttackType_Heavy))
	{
		GroggyDamage = CombatComponent->GetCurrentWeaponHeavyAttackGroggyDamage();
	}

	const FGameplayEffectSpecHandle SpecHandle = CreateDamageEffectSpec(BaseDamage, GroggyDamage);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 콤보 횟수를 전달 -> 계산기에서 콤보 횟수에 비례한 데미지 보너스 적용 (플레이어 전용)
	if (bApplyComboDamageBonus && ComboAttackTypeTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ComboAttackTypeTag, static_cast<float>(CurrentComboCount));
	}

	// 카운터 어택 보너스 배율 전달 -> 계산기에서 FinalDamage에 곱한다
	if (bIsCounterAttack)
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_CounterAttackBonus, CounterAttackDamageMultiplier);
	}

	if (ApplyDamageEffectSpecToTarget(SpecHandle, HitActor, BaseDamage))
	{
		PlayHitGameplayCue(HitActor);
	}

	if (HitCameraShakeClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()))
		{
			PlayerController->ClientStartCameraShake(HitCameraShakeClass);
		}
	}

	ApplyAdditionalHitEffects(HitActor, Payload);
}

UAOEDamageComponent* UACAbility_Attack::GetOrCreateAOEDamageComponent() const
{
	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	if (UAOEDamageComponent* Existing = OwnerCharacter->FindComponentByClass<UAOEDamageComponent>())
	{
		return Existing;
	}

	UAOEDamageComponent* NewComponent = NewObject<UAOEDamageComponent>(OwnerCharacter);
	NewComponent->RegisterComponent();
	return NewComponent;
}

void UACAbility_Attack::OnInstantAOEEventReceived(FGameplayEventData Payload)
{
	// 서버 권한에서만 판정한다 — 클라이언트에서의 중복 판정/데미지 적용을 방지한다.
	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	const UPawnCombatComponent* CombatComponent = OwnerCharacter ? OwnerCharacter->GetPawnCombatComponent() : nullptr;
	UAOEDamageComponent* AOEComponent = GetOrCreateAOEDamageComponent();

	if (!CombatComponent || !AOEComponent || !DamageEffect)
	{
		return;
	}

	// virtual dispatch를 통해 Player/Enemy 각자의 DataTable 값을 가져온다
	const float BaseDamage = CombatComponent->GetCurrentWeaponBaseDamage() * AOEBaseDamageMultiplier;
	const float GroggyDamage = AOEGroggyDamage;

	AOEComponent->TriggerInstantAOE(InstantAOERadius, InstantAOEForwardOffset, bDebugDrawAOE,
		[this, OwnerCharacter, BaseDamage, GroggyDamage](AActor* TargetActor)
		{
			// 무기 콜리전 근접 공격과 동일하게, 유효한 블록이면 대상에게 Block/Parry GameplayCue를 발동시킨다.
			UACFunctionLibrary::TryTriggerSuccessfulBlockEvent(OwnerCharacter, TargetActor);

			const FGameplayEffectSpecHandle SpecHandle = CreateDamageEffectSpec(BaseDamage, GroggyDamage);
			if (ApplyDamageEffectSpecToTarget(SpecHandle, TargetActor, BaseDamage))
			{
				PlayHitGameplayCue(TargetActor);
			}
		});
}

void UACAbility_Attack::OnSustainedAOEStartReceived(FGameplayEventData Payload)
{
	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	const UPawnCombatComponent* CombatComponent = OwnerCharacter ? OwnerCharacter->GetPawnCombatComponent() : nullptr;
	UAOEDamageComponent* AOEComponent = GetOrCreateAOEDamageComponent();

	if (!CombatComponent || !AOEComponent || !DamageEffect)
	{
		return;
	}

	// virtual dispatch를 통해 Player/Enemy 각자의 DataTable 값을 가져온다
	const float BaseDamage = CombatComponent->GetCurrentWeaponBaseDamage() * AOEBaseDamageMultiplier;
	const float GroggyDamage = AOEGroggyDamage;

	AOEComponent->StartSustainedAOE(SustainedAOERadius, SustainedAOEForwardOffset, SustainedAOEDamageInterval, bDebugDrawAOE,
		[this, OwnerCharacter, BaseDamage, GroggyDamage](AActor* TargetActor)
		{
			// 무기 콜리전 근접 공격과 동일하게, 유효한 블록이면 대상에게 Block/Parry GameplayCue를 발동시킨다.
			UACFunctionLibrary::TryTriggerSuccessfulBlockEvent(OwnerCharacter, TargetActor);

			const FGameplayEffectSpecHandle SpecHandle = CreateDamageEffectSpec(BaseDamage, GroggyDamage);
			if (ApplyDamageEffectSpecToTarget(SpecHandle, TargetActor, BaseDamage))
			{
				PlayHitGameplayCue(TargetActor);
			}
		});
}

void UACAbility_Attack::OnSustainedAOEEndReceived(FGameplayEventData Payload)
{
	if (AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo())
	{
		if (UAOEDamageComponent* AOEComponent = OwnerCharacter->FindComponentByClass<UAOEDamageComponent>())
		{
			AOEComponent->StopSustainedAOE();
		}
	}
}