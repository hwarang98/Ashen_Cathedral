// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UACAttributeSet::UACAttributeSet()
{
	//Core
	InitMoveSpeed(400.f);
	InitHealth(1.f);
	InitMaxHealth(1.f);
	InitStamina(1.f);
	InitMaxStamina(1.f);
	InitStaminaRegenRate(1.f);

	// Groggy
	InitGroggyGauge(1.f);
	InitMaxGroggyGauge(1.f);
	InitGroggyResistance(1.f);
	InitGroggyDamageTaken(0.f);

	// Combat
	InitAttackPower(1.f); // 배율
	InitDefensePower(0.f);
	InitAttackSpeed(1.f);
	InitDamageTaken(0.f);
	InitStaminaCost(0.f);
}

void UACAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Core — 현재값은 0과 최대값 사이로 클램프
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetStaminaRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Groggy — 게이지는 0과 최대값 사이로 클램프
	else if (Attribute == GetGroggyGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxGroggyGauge());
	}
	else if (Attribute == GetGroggyResistanceAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Combat — 음수 방지
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetDefensePowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UACAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// Periodic GE가 BaseValue를 무한정 올리지 않도록 클램프
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetDamageTakenAttribute())
	{
		HandleDamageAndTriggerHitReact(Data);
	}
	else if (Data.EvaluatedData.Attribute == GetGroggyDamageTakenAttribute())
	{
		HandleGroggyDamage(Data);
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaCostAttribute())
	{
		HandleStaminaConsumption(Data);
	}
}


void UACAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = NewValue;
			}
		}
	}
}

void UACAttributeSet::HandleGroggyDamage(const FGameplayEffectModCallbackData& Data)
{
	const float GroggyDamage = GetGroggyDamageTaken();
	SetGroggyDamageTaken(0.f);

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	// 사망 상태라면 그로기 누적 불필요
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	// 이미 그로기 상태라면 누적하지 않음
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Groggy))
	{
		return;
	}

	// 무적or슈퍼아머 태그 보유시 그로기 데미지 무효화
	static FGameplayTagContainer GroggyImmunityTags;
	if (GroggyImmunityTags.IsEmpty())
	{
		GroggyImmunityTags.AddTag(ACGameplayTags::Shared_Status_Invincible);
		GroggyImmunityTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor);
	}

	if (TargetASC && TargetASC->HasAnyMatchingGameplayTags(GroggyImmunityTags))
	{
		return;
	}

	const float ReducedDamage = FMath::Max(GroggyDamage - GetGroggyResistance(), 0.f);
	const float OldGroggy = GetGroggyGauge();
	const float NewGroggy = FMath::Clamp(OldGroggy + ReducedDamage, 0.f, GetMaxGroggyGauge());
	SetGroggyGauge(NewGroggy);

	if (NewGroggy >= GetMaxGroggyGauge())
	{
		// GA_Groggy를 통해 그로기 상태 처리
		FGameplayEventData Payload;
		Payload.EventTag = ACGameplayTags::Shared_Event_GroggyTriggered;
		Payload.Target = Data.Target.GetAvatarActor();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Data.Target.GetAvatarActor(),
			ACGameplayTags::Shared_Event_GroggyTriggered,
			Payload
			);
	}
}

void UACAttributeSet::HandleDamageAndTriggerHitReact(const FGameplayEffectModCallbackData& Data)
{
	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	// 무적 태그 보유시 데미지 무효화
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible))
	{
		SetDamageTaken(0.f);
		return;
	}

	// 이미 사망 상태라면 중복 처리 방지
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		SetDamageTaken(0.f);
		return;
	}

	const float DamageDone = GetDamageTaken();
	SetDamageTaken(0.f);

	if (DamageDone <= 0.f)
	{
		return;
	}

	const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.f, GetMaxHealth());
	SetHealth(NewHealth);

	// 사망 판정
	if (GetHealth() <= 0.f)
	{
		UACFunctionLibrary::AddGameplayTagToActorIfNone(
			Data.Target.GetAvatarActor(),
			ACGameplayTags::Shared_Status_Dead
			);

		// 사망 이벤트 전송 (Death Drop Ability 트리거용)
		FGameplayEventData DeathPayload;
		DeathPayload.EventTag = ACGameplayTags::Shared_Event_Death;
		DeathPayload.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
		DeathPayload.Target = Data.Target.GetAvatarActor();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Data.Target.GetAvatarActor(),
			ACGameplayTags::Shared_Event_Death,
			DeathPayload
			);

		return;
	}

	// HitReact — 무적·슈퍼아머 보유 시 무효화
	static FGameplayTagContainer HitReactImmunityTags;
	if (HitReactImmunityTags.IsEmpty())
	{
		HitReactImmunityTags.AddTag(ACGameplayTags::Shared_Status_Invincible);
		HitReactImmunityTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor);
	}

	if (TargetASC && !TargetASC->HasAnyMatchingGameplayTags(HitReactImmunityTags))
	{
		FGameplayEventData HitPayload;
		HitPayload.EventTag = ACGameplayTags::Shared_Event_HitReact;
		HitPayload.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
		HitPayload.Target = Data.Target.GetAvatarActor();
		HitPayload.EventMagnitude = DamageDone;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Data.Target.GetAvatarActor(),
			ACGameplayTags::Shared_Event_HitReact,
			HitPayload
			);
	}
}

void UACAttributeSet::HandleStaminaConsumption(const FGameplayEffectModCallbackData& Data)
{
	const float Cost = GetStaminaCost();
	SetStaminaCost(0.f); // 메타 초기화

	if (Cost <= 0.f)
	{
		return;
	}

	// Stamina 실제 감소
	const float NewStamina = FMath::Clamp(GetStamina() - Cost, 0.f, GetMaxStamina());
	SetStamina(NewStamina);

	// 딜레이 GE 재적용 — GE의 Stacking(Refresh on Successful Application)이 Duration을 자동 리셋
	UACAbilitySystemComponent* ASC = Cast<UACAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (!ASC || !ASC->StaminaRegenDelayEffectClass)
	{
		return;
	}

	const UGameplayEffect* DelayGE = ASC->StaminaRegenDelayEffectClass->GetDefaultObject<UGameplayEffect>();
	ASC->ApplyGameplayEffectToSelf(DelayGE, 1, ASC->MakeEffectContext());
}