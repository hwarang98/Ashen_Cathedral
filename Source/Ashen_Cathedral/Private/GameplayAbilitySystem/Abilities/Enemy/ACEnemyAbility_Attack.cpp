// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACEnemyAbility_Attack::UACEnemyAbility_Attack()
{
	// 적은 콤보 횟수에 따른 데미지 증가 없음
	bApplyComboDamageBonus = false;
}

void UACEnemyAbility_Attack::HandleComboComplete()
{
	// BT가 공격 타이밍을 제어하므로 콤보 카운트 리셋을 하지 않는다.
	// HandleComboCancelled는 베이스 클래스(즉시 리셋)를 그대로 사용
}

UAnimMontage* UACEnemyAbility_Attack::SelectAttackMontage()
{
	const int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);
	return AttackMontages[RandomIndex];
}

void UACEnemyAbility_Attack::ModifyDamageSpec(const FGameplayEffectSpecHandle& SpecHandle, const AActor* HitActor, float BaseDamage)
{
	const UACAbilitySystemComponent* OwnerASC = GetACAbilitySystemComponentFromActorInfo();
	if (!OwnerASC || !OwnerASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_State_Phase2))
	{
		return;
	}

	// TODO: 난이도 시스템 연동 필요
	const float DifficultyLevel = GetAbilityLevel();

	// 화염 추가 데미지: BaseDamage * ScalableFloat(DifficultyLevel)
	{
		const float Multiplier = FireBonusDamageMultiplierCurve.GetValueAtLevel(DifficultyLevel);
		if (Multiplier > 0.f)
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_FireBonusDamage, BaseDamage * Multiplier);
		}
	}

	// 화상 축적량: ScalableFloat(DifficultyLevel)
	{
		const float BurnBuildUp = BurnBuildUpAmountCurve.GetValueAtLevel(DifficultyLevel);
		if (BurnBuildUp > 0.f)
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_BurnBuildUp, BurnBuildUp);
		}
	}

	// 그로기 데미지 배율: 기존 Spec의 GroggyDamage × ScalableFloat(DifficultyLevel)
	{
		const float GroggyMultiplier = GroggyDamageMultiplierCurve.GetValueAtLevel(DifficultyLevel);
		if (GroggyMultiplier > 0.f)
		{
			const float CurrentGroggyDamage = SpecHandle.Data->GetSetByCallerMagnitude(ACGameplayTags::Shared_SetByCaller_GroggyDamage, false, 0.f);
			if (CurrentGroggyDamage > 0.f)
			{
				UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_GroggyDamage, CurrentGroggyDamage * GroggyMultiplier);
			}
		}
	}
}