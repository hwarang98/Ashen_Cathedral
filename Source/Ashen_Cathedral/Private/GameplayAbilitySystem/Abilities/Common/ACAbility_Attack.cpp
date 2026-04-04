// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ACGameplayTags.h"
#include "Camera/CameraShakeBase.h"
#include "Character/ACCharacterBase.h"
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

	const AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	UPawnCombatComponent* CombatComponent = OwnerCharacter ? OwnerCharacter->GetPawnCombatComponent() : nullptr;
	if (!CombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	// Shared_Status_CanCounterAttack 태그가 있으면 카운터 어택으로 처리
	const bool bIsCounterAttack = ASC && ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_CanCounterAttack);

	UAnimMontage* MontageToPlay;

	if (bIsCounterAttack && CounterAttackMontage)
	{
		// 카운터 어택: 전용 몽타주 재생 및 콤보 카운트 초기화
		MontageToPlay = CounterAttackMontage;
		CurrentComboCount = 0;
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
	if (!bIsCounterAttack)
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

void UACAbility_Attack::HandleComboCancelled()
{
	ResetComboCount();
}

void UACAbility_Attack::ResetComboCount()
{
	CurrentComboCount = 0;
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

	// bWasCancelled=true → EndAbility 내부에서 HandleComboCancelled 호출
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
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
	const bool bIsCounterAttack = ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_CanCounterAttack);

	float GroggyDamage = 0.f;
	if (bIsCounterAttack)
	{
		GroggyDamage = CombatComponent->GetCurrentWeaponCounterAttackGroggyDamage();
	}
	else if (ComboAttackTypeTag.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_AttackType_Heavy))
	{
		GroggyDamage = CombatComponent->GetCurrentWeaponHeavyAttackGroggyDamage();
	}

	// Effect Spec 생성: DamageEffect(GE)의 껍데기를 만든다.
	// 이 시점에는 SetByCaller 값이 비어있다.
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// SetByCaller: 태그를 키로 동적 값을 Spec에 주입한다.
	// ACCalculation_DamageTaken에서 이 태그들을 키로 값을 꺼내 최종 데미지를 계산한다.
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_BaseDamage, BaseDamage);

	// 콤보 횟수를 전달 -> 계산기에서 콤보 횟수에 비례한 데미지 보너스 적용
	if (ComboAttackTypeTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ComboAttackTypeTag, static_cast<float>(CurrentComboCount));
	}

	if (GroggyDamage > 0.f)
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_GroggyDamage, GroggyDamage);
	}

	// 카운터 어택 보너스 배율 전달 -> 계산기에서 FinalDamage에 곱한다
	if (bIsCounterAttack)
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_CounterAttackBonus, CounterAttackDamageMultiplier);
	}

	// Spec을 타겟 ASC에 적용한다.
	// 적용 순서: ApplyGameplayEffectSpecToTarget
	//           → ACCalculation_DamageTaken::Execute (SetByCaller 값으로 최종 데미지 계산)
	//           → ACAttributeSet::PostGameplayEffectExecute (DamageTaken → Health 차감, GroggyGauge 증가)
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(HitActor)))
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = OwnerCharacter;
		CueParams.EffectCauser = OwnerCharacter;
		CueParams.SourceObject = Cast<AACWeaponBase>(CombatComponent->GetCharacterCurrentEquippedWeapon()); // Player/Enemy 모두 사용 가능한 PawnCombatComponent 함수 사용
		CueParams.TargetAttachComponent = HitActor->GetRootComponent();
		CueParams.Location = HitActor->GetActorLocation();
		CueParams.Normal = (OwnerCharacter->GetActorLocation() - HitActor->GetActorLocation()).GetSafeNormal();

		ASC->ExecuteGameplayCue(MeleeAttackSoundCueTag, CueParams);
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}

	if (HitCameraShakeClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()))
		{
			PlayerController->ClientStartCameraShake(HitCameraShakeClass);
		}
	}
}