// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/ACCharacterBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Controllers/ACEnemyController.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/GameplayEffects/ACGameplayEffect_DynamicCooldown.h"
#include "GameplayTags/ACGameplayTags_Shared.h"
#include "Runtime/Media/Public/IMediaControls.h"

#if AC_WEB_DEBUG
	#include "Debug/ACWebDebugSubsystem.h"
#endif

void UACGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (AbilityActivationPolicy == EACAbilityActivationPolicy::OnGiven)
	{
		// 어빌리티가 아직 활성화되지 않았다면
		if (ActorInfo && !Spec.IsActive())
		{
			// 어빌리티 활성화를 시도 
			// 이 함수는 내부적으로 CanActivateAbility를 호출하므로 서버/클라 권한 체크가 자동으로 수행
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

#if AC_WEB_DEBUG
void UACGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(Avatar))
	{
		WebDebug->RecordAbility(Avatar, GetClass()->GetName(), EACWebDebugPhase::Begin);
	}
}

void UACGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// begin/end 짝을 정확히 맞추기 위해, 실제로 활성 상태였을 때만 end 를 남긴다
	if (IsActive())
	{
		AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(Avatar))
		{
			WebDebug->RecordAbility(Avatar, GetClass()->GetName(), EACWebDebugPhase::End);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
#endif

UACAbilitySystemComponent* UACGameplayAbility::GetACAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UACAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AACCharacterBase* UACGameplayAbility::GetACCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

AACPlayerCharacter* UACGameplayAbility::GetACPlayerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACPlayerCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

AACEnemyCharacter* UACGameplayAbility::GetACEnemyFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AACEnemyCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UPawnCombatComponent* UACGameplayAbility::GetPawnCombatComponentFromActorInfo() const
{
	if (const AACCharacterBase* CharacterBase = GetACCharacterFromActorInfo())
	{
		return CharacterBase->GetPawnCombatComponent();
	}

	return nullptr;
}

const FGameplayTagContainer* UACGameplayAbility::GetCooldownTags() const
{
	return CooldownIdentifierTags.Num() > 0 ? &CooldownIdentifierTags : nullptr;
}

UGameplayEffect* UACGameplayAbility::GetCooldownGameplayEffect() const
{
	return UACGameplayEffect_DynamicCooldown::StaticClass()->GetDefaultObject<UGameplayEffect>();
}

void UACGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownIdentifierTags.IsEmpty() || CooldownDurationSeconds <= 0.f)
	{
		return;
	}

	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!ensure(CooldownGE))
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		ensureMsgf(false, TEXT("Failed to create Cooldown GameplayEffectSpec"));
		return;
	}

	// 쿨다운 식별 태그를 런타임에 부여한다 — GE는 태그를 정적으로 갖지 않으므로 어빌리티마다 독립 쿨다운이 된다
	SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownIdentifierTags);
	// SetByCaller로 지속시간을 넘긴다
	SpecHandle.Data->SetSetByCallerMagnitude(ACGameplayTags::Shared_SetByCaller_CooldownDuration, CooldownDurationSeconds);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}