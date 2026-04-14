// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Calculation/ACCalculation_DamageTaken.h"
#include "AbilitySystemComponent.h"
#include "ACGameplayTags.h"
#include "Structs/ACStructTypes.h"

static FCADamageCapture& GetDamageCapture()
{
	static FCADamageCapture DamageCapture;
	return DamageCapture;
}

UACCalculation_DamageTaken::UACCalculation_DamageTaken()
{
	RelevantAttributesToCapture.Add(GetDamageCapture().AttackPowerDef);
	RelevantAttributesToCapture.Add(GetDamageCapture().DefensePowerDef);
	RelevantAttributesToCapture.Add(GetDamageCapture().DamageTakenDef);
	RelevantAttributesToCapture.Add(GetDamageCapture().GroggyDamageTakenDef);
	RelevantAttributesToCapture.Add(GetDamageCapture().BurnAccumulationDef);
}

void UACCalculation_DamageTaken::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();
	FAggregatorEvaluateParameters EvaluateParameters;

	EvaluateParameters.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	float SourceAttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetDamageCapture().AttackPowerDef,
		EvaluateParameters,
		SourceAttackPower
		);

	// SetByCaller로 전달된 동적 값들 가져오기
	float BaseDamage = 0.f;
	float BaseGroggyDamage = 0.f;
	float CounterAttackBonus = 0.f; // 카운터 공격이 아니면 0
	float FireBonusDamage = 0.f;
	float BurnBuildUp = 0.f;
	int32 UsedLightAttackComboCount = 0;
	int32 UsedHeavyAttackComboCount = 0;

	for (const TPair<FGameplayTag, float>& TagMagnitude : EffectSpec.SetByCallerTagMagnitudes)
	{
		const float MagnitudeValue = TagMagnitude.Value;

		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_BaseDamage))
		{
			BaseDamage = MagnitudeValue;
		}
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_AttackType_Light))
		{
			UsedLightAttackComboCount = MagnitudeValue;
		}
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_AttackType_Heavy))
		{
			UsedHeavyAttackComboCount = MagnitudeValue;
		}
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_CounterAttackBonus))
		{
			CounterAttackBonus = MagnitudeValue;
		}
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_GroggyDamage))
		{
			BaseGroggyDamage = MagnitudeValue;
		}
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_FireBonusDamage))
		{
			FireBonusDamage = MagnitudeValue;
		}
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_BurnBuildUp))
		{
			BurnBuildUp = MagnitudeValue;
		}
	}

	float TargetDefensePower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetDamageCapture().DefensePowerDef,
		EvaluateParameters,
		TargetDefensePower
		);

	if (UsedLightAttackComboCount != 0)
	{
		BaseDamage *= (UsedLightAttackComboCount - 1) * 0.05f + 1.f;
	}

	if (UsedHeavyAttackComboCount != 0)
	{
		BaseDamage *= UsedHeavyAttackComboCount * 0.15f + 1.f;
	}

	// AttackPower와 DefensePower를 정규화된 값으로 해석
	// 예: AttackPower가 1.0이면 100% (기본), 1.5면 150% (1.5배)
	// 예: DefensePower가 0.6이면 60% 감소, 최종 데미지는 40%만 받음
	// 최대 방어율은 95%로 제한
	const float AttackMultiplier = SourceAttackPower;
	const float DefenseMultiplier = FMath::Clamp(TargetDefensePower, 0.f, 0.95f);

	// FireBonusDamage는 BaseDamage에 합산 후 AttackMultiplier / Defense 계산 적용
	float FinalDamageDone = (BaseDamage + FireBonusDamage) * AttackMultiplier * (1.0f - DefenseMultiplier);

	// 카운터 공격 보너스 적용 (설정되어 있을 때만)
	if (CounterAttackBonus > 0.f)
	{
		FinalDamageDone *= CounterAttackBonus;
	}

	// 패링/블락 상태 데미지 보정
	if (const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
	{
		if (TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Parry))
		{
			FinalDamageDone = 0.f;
		}
		else if (TargetASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_Blocking))
		{
			FinalDamageDone *= 0.3f;
		}
	}

	// if (GEngine)
	// {
	// 	const FString DebugMsg = FString::Printf(
	// 		TEXT("[ 데미지 계산 ]\n경량 콤보: %d회 | 중량 콤보: %d회\n기본 데미지: %.1f | 화염 추가: %.1f | 공격력: %.2f | 방어율: %.0f%%\n카운터 보너스: %.2fx\n─────────────────\n최종 데미지: %.1f | 화상 축적: %.2f\n─────────────────\n"),
	// 		UsedLightAttackComboCount,
	// 		UsedHeavyAttackComboCount,
	// 		BaseDamage,
	// 		FireBonusDamage,
	// 		AttackMultiplier,
	// 		DefenseMultiplier * 100.f,
	// 		CounterAttackBonus > 0.f ? CounterAttackBonus : 1.f,
	// 		FinalDamageDone,
	// 		BurnBuildUp
	// 		);
	//
	// 	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, DebugMsg);
	// 	UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugMsg);
	// }

	// 계산된 데미지를 출력(Output)으로 설정
	if (FinalDamageDone > 0.f)
	{
		const FGameplayModifierEvaluatedData ModifierEvaluatedData = FGameplayModifierEvaluatedData(
			GetDamageCapture().DamageTakenProperty,
			EGameplayModOp::Additive,
			FinalDamageDone
			);

		// AttributeSet의 PostGameplayEffectExecute에서 이 DamageTaken 값을 읽어 CurrentHealth를 감소
		OutExecutionOutput.AddOutputModifier(ModifierEvaluatedData);
	}

	// 그로기 누적
	if (BaseGroggyDamage > 0.f)
	{
		const FGameplayModifierEvaluatedData GroggyModifier(
			GetDamageCapture().GroggyDamageTakenProperty,
			EGameplayModOp::Additive,
			BaseGroggyDamage
			);

		OutExecutionOutput.AddOutputModifier(GroggyModifier);
	}

	// 화상 축적 — BurnAccumulation 메타 Attribute에 출력, PostGameplayEffectExecute에서 BurnGauge에 반영
	if (BurnBuildUp > 0.f)
	{
		const FGameplayModifierEvaluatedData BurnModifier(
			GetDamageCapture().BurnAccumulationProperty,
			EGameplayModOp::Additive,
			BurnBuildUp
			);

		OutExecutionOutput.AddOutputModifier(BurnModifier);
	}
}