// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACEnemyAbility_Attack.generated.h"

/**
 * 적 전용 공격 어빌리티.
 * 콤보 흐름은 Behavior Tree가 제어하므로 HandleComboComplete는 아무것도 하지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_Attack : public UACAbility_Attack
{
	GENERATED_BODY()

public:
	UACEnemyAbility_Attack();

	/** BT가 다음 공격을 결정하므로 콤보 완료 시 아무것도 하지 않는다. */
	virtual void HandleComboComplete() override;

	/** AttackMontages 배열에서 무작위로 몽타주를 선택해 반환한다. */
	virtual UAnimMontage* SelectAttackMontage() override;

protected:
	/**
	 * @brief Enemy_State_Phase2 태그 보유 시 동일 DamageEffect Spec에 화염 데미지와 화상 축적 값을 주입한다.
	 * DamageCalculation에서 Shared.SetByCaller.FireBonusDamage / BurnBuildUp 태그로 읽어 처리한다.
	 *
	 * @param SpecHandle  빌드 중인 DamageEffect Spec 핸들
	 * @param HitActor    적중된 대상 액터
	 * @param BaseDamage  이 공격의 기본 데미지 값
	 */
	virtual void ModifyDamageSpec(const FGameplayEffectSpecHandle& SpecHandle, const AActor* HitActor, float BaseDamage) override;

private:
	/**
	 * Phase2 화염 추가 데미지 배율 커브.
	 * FireBonusDamage = BaseDamage * Curve(AbilityLevel)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2", meta = (AllowPrivateAccess = "true"))
	FScalableFloat FireBonusDamageMultiplierCurve;

	/**
	 * Phase2 공격 1회당 화상 축적량 커브.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2", meta = (AllowPrivateAccess = "true"))
	FScalableFloat BurnBuildUpAmountCurve;

	/**
	 * Phase2 그로기 데미지 배율 커브.
	 * 기존 GroggyDamage에 이 배율을 곱해 Phase2 강화 그로기를 적용합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2", meta = (AllowPrivateAccess = "true"))
	FScalableFloat GroggyDamageMultiplierCurve;
};
