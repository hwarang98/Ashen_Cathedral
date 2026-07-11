// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Calculation/ACCalculation_DamageTaken.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "GameplayAbilitySystem/GameplayEffects/ACGameplayEffect_PostureCounter.h"
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
	RelevantAttributesToCapture.Add(GetDamageCapture().PostureDamageTakenDef);
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
	float BasePostureDamage = 0.f;
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
		if (TagMagnitude.Key.MatchesTagExact(ACGameplayTags::Shared_SetByCaller_PostureDamage))
		{
			BasePostureDamage = MagnitudeValue;
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

	// 패링/블록 상태에 따른 데미지 보정 및 체간(Posture) 처리
	// - 패링 성공: 피격 데미지 0, 방어자 체간 누적 없음, 공격자(Source)에게 체간 역공(BasePostureDamage * 1.5) 전송
	// - 블록 성공: 피격 데미지 90% 감소, 방어자 본인에게 체간 데미지(BasePostureDamage * 0.8) 누적
	float FinalPostureDamage = BasePostureDamage;
	if (const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
	{
		if (TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Parry))
		{
			FinalDamageDone = 0.f;
			FinalPostureDamage = 0.f;

			// Execution 출력(AddOutputModifier)은 Target만 수정 가능하므로, Source(공격자) 본인에게는
			// 체간 역공 GE(UACGameplayEffect_PostureCounter)를 직접 적용한다.
			// 흡혈/반사 데미지와 동일하게 널리 쓰이는 GAS 패턴이며, 어빌리티 부여 여부와 무관하게
			// ASC를 가진 모든 액터에 예외 없이 적용되므로 부여 누락에 의한 사일런트 실패가 없다.
			if (BasePostureDamage > 0.f)
			{
				if (const UAbilitySystemComponent* SourceASCConst = ExecutionParams.GetSourceAbilitySystemComponent())
				{
					if (AActor* SourceActor = SourceASCConst->GetAvatarActor())
					{
						if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor))
						{
							const FGameplayEffectSpecHandle CounterSpecHandle = SourceASC->MakeOutgoingSpec(UACGameplayEffect_PostureCounter::StaticClass(), 1.f, SourceASC->MakeEffectContext());
							if (CounterSpecHandle.IsValid())
							{
								UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(CounterSpecHandle, ACGameplayTags::Shared_SetByCaller_PostureDamage, BasePostureDamage * 1.5f);
								// 패링 역공은 카운터 성격이므로, 공격 중(SuperArmor) 상태인 공격자에게도 체간 데미지가 적용되도록
								// CounterAttackBonus SetByCaller를 부여해 HandlePostureDamage의 슈퍼아머 무효화 가드를 우회한다.
								UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(CounterSpecHandle, ACGameplayTags::Shared_SetByCaller_CounterAttackBonus, 1.f);
								SourceASC->ApplyGameplayEffectSpecToSelf(*CounterSpecHandle.Data.Get());
							}
						}
					}
				}
			}
		}
		else if (TargetASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_Blocking))
		{
			FinalDamageDone *= 0.1f;
			FinalPostureDamage = BasePostureDamage * 0.8f;
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

	// 체간 누적 (패링/블록 보정이 반영된 최종 체간 데미지)
	if (FinalPostureDamage > 0.f)
	{
		const FGameplayModifierEvaluatedData PostureModifier(
			GetDamageCapture().PostureDamageTakenProperty,
			EGameplayModOp::Additive,
			FinalPostureDamage
			);

		OutExecutionOutput.AddOutputModifier(PostureModifier);
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