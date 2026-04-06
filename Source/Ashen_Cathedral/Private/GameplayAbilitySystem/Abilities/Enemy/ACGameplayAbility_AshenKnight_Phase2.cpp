// Ashen Knight Phase2 진입 어빌리티. 화염 강화 상태로 전환하며 되돌아가지 않습니다.

#include "GameplayAbilitySystem/Abilities/Enemy/ACGameplayAbility_AshenKnight_Phase2.h"
#include "ACGameplayTags.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Items/Weapon/ACWeaponBase.h"

UACGameplayAbility_AshenKnight_Phase2::UACGameplayAbility_AshenKnight_Phase2()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 활성화 중 Enemy.State.Phase2 태그를 ASC에 부여한다.
	// 동일 태그가 ActivationBlockedTags에도 있으므로 중복 실행이 자동 차단된다.
	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_State_Phase2);
	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_State_Phase2);

	// 기본 이벤트 태그. BP에서 덮어쓸 수 있다.
	VisualActivateEventTag = ACGameplayTags::Enemy_Event_Phase2_VisualActivate;
}

void UACGameplayAbility_AshenKnight_Phase2::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 스탯 강화 GE 즉시 적용 (Infinite — 몽타주와 무관하게 바로 발동)
	ApplyPhase2StatsEffect(Handle, ActorInfo, ActivationInfo);

	// 2. 몽타주가 설정된 경우: 재생 후 이벤트 수신 시 비주얼 적용
	if (Phase2TransitionMontage)
	{
		// 몽타주 재생 중 무적 태그 부여 (Shared.Status.Invincible → AttributeSet에서 데미지 무효화)
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->AddLooseGameplayTag(ACGameplayTags::Shared_Status_Invincible);
		}

		// AI 이동 정지 — BT 일시 중단 + 현재 이동 경로 취소
		if (AAIController* AIC = GetOwningAIController())
		{
			AIC->StopMovement();
			if (AIC->BrainComponent)
			{
				AIC->BrainComponent->PauseLogic(TEXT("Phase2Transition"));
			}
		}

		// VisualActivateEventTag가 비어있으면 즉시 비주얼 적용 후 반환
		if (!VisualActivateEventTag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Phase2] VisualActivateEventTag가 설정되지 않았습니다. 즉시 비주얼을 적용합니다: %s"), *GetName());
			OnVisualActivateEventReceived(FGameplayEventData{});
			return;
		}

		// 이벤트 대기 태스크 먼저 등록 (몽타주 재생과 동시에 수신 준비)
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			VisualActivateEventTag,
			nullptr, // OptionalExternalTarget — 자기 자신 ASC
			true,    // OnlyTriggerOnce
			true     // bAsyncronous
			);
		WaitEventTask->EventReceived.AddDynamic(this, &UACGameplayAbility_AshenKnight_Phase2::OnVisualActivateEventReceived);
		WaitEventTask->ReadyForActivation();

		// 몽타주 재생 태스크
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			Phase2TransitionMontage
			);
		// 몽타주 종료/중단/취소 시 무적 태그 제거
		// Phase2는 영구 상태이므로 EndAbility는 호출하지 않는다.
		MontageTask->OnCompleted.AddDynamic(this, &UACGameplayAbility_AshenKnight_Phase2::OnPhase2MontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &UACGameplayAbility_AshenKnight_Phase2::OnPhase2MontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &UACGameplayAbility_AshenKnight_Phase2::OnPhase2MontageEnded);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// 몽타주 없음 → 즉시 비주얼 적용
		OnVisualActivateEventReceived(FGameplayEventData{});
	}

	// Phase2는 되돌아가지 않으므로 EndAbility를 호출하지 않는다.
}

void UACGameplayAbility_AshenKnight_Phase2::OnVisualActivateEventReceived(FGameplayEventData Payload)
{
	AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		return;
	}

	// 캐릭터 메시 화염 머티리얼 교체 (cloth, Arm, Greaves, Body, cape — 5슬롯)
	ApplyFireMaterial(Mesh);

	// 무기 머티리얼 교체 + Niagara 부착
	UPawnCombatComponent* CombatComp = GetPawnCombatComponentFromActorInfo();
	if (CombatComp)
	{
		AACWeaponBase* Weapon = CombatComp->GetCharacterCurrentEquippedWeapon();
		ApplyFireMaterialToWeapon(Weapon);
		AttachNiagaraEffectsToWeapon(Weapon);
	}

	// 캐릭터 메시 Niagara 이펙트 부착
	AttachNiagaraEffects(Mesh);
}

void UACGameplayAbility_AshenKnight_Phase2::OnPhase2MontageEnded()
{
	// 무적 해제
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Shared_Status_Invincible);
	}

	// AI BT 재개
	if (AAIController* AIC = GetOwningAIController())
	{
		if (AIC->BrainComponent)
		{
			AIC->BrainComponent->ResumeLogic(TEXT("Phase2Transition"));
		}
	}
}

AAIController* UACGameplayAbility_AshenKnight_Phase2::GetOwningAIController() const
{
	const AACCharacterBase* OwnerCharacter = GetACCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		return nullptr;
	}
	return Cast<AAIController>(OwnerCharacter->GetController());
}

void UACGameplayAbility_AshenKnight_Phase2::ApplyPhase2StatsEffect(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const
{
	if (!Phase2StatsEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Phase2] Phase2StatsEffect가 설정되지 않았습니다. 스탯 강화가 적용되지 않습니다: %s"), *GetName());
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Phase2StatsEffect, GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

void UACGameplayAbility_AshenKnight_Phase2::ApplyFireMaterial(USkeletalMeshComponent* Mesh) const
{
	for (int32 i = 0; i < FireMaterials.Num(); ++i)
	{
		// 해당 슬롯 머티리얼이 None이면 원본 유지, 유효하면 교체
		if (FireMaterials[i] && i < Mesh->GetNumMaterials())
		{
			Mesh->SetMaterial(i, FireMaterials[i]);
		}
	}
}

void UACGameplayAbility_AshenKnight_Phase2::ApplyFireMaterialToWeapon(AACWeaponBase* Weapon) const
{
	if (!Weapon || !FireMaterialWeapon)
	{
		return;
	}

	UMeshComponent* WeaponMesh = Weapon->GetWeaponMeshComponent();
	if (!WeaponMesh)
	{
		return;
	}

	const int32 NumMaterials = WeaponMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		WeaponMesh->SetMaterial(i, FireMaterialWeapon);
	}
}

void UACGameplayAbility_AshenKnight_Phase2::AttachNiagaraEffectsToWeapon(AACWeaponBase* Weapon) const
{
	if (!Weapon)
	{
		return;
	}

	UMeshComponent* WeaponMesh = Weapon->GetWeaponMeshComponent();
	if (!WeaponMesh)
	{
		return;
	}

	for (const FACPhase2NiagaraAttachment& Attachment : NiagaraAttachmentsWeapon)
	{
		if (!Attachment.NiagaraSystem)
		{
			continue;
		}

		// SocketName이 None이면 무기 메시 루트에 부착한다.
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			Attachment.NiagaraSystem,
			WeaponMesh,
			Attachment.SocketName, // NAME_None → 컴포넌트 루트 기준 부착
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false // bAutoDestroy
			);
	}
}

void UACGameplayAbility_AshenKnight_Phase2::AttachNiagaraEffects(USkeletalMeshComponent* Mesh) const
{
	for (const FACPhase2NiagaraAttachment& Attachment : NiagaraAttachments)
	{
		if (!Attachment.NiagaraSystem || Attachment.SocketName.IsNone())
		{
			continue;
		}

		// bAutoDestroy=false: Phase2는 영구 상태이므로 이펙트가 루핑으로 계속 재생되어야 한다.
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			Attachment.NiagaraSystem,
			Mesh,
			Attachment.SocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false // bAutoDestroy
			);
	}
}
