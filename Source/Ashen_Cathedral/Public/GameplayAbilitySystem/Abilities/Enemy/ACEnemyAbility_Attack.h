// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"
#include "Enums/ACEnums.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACEnemyAbility_Attack.generated.h"

/**
 * 콤보 한 벌의 정의. Montages를 배열 순서대로 한 스텝씩 재생한다.
 * ComboSequence 모드에서 여러 콤보 중 하나를 Weight 비율로 뽑아 사용한다.
 */
USTRUCT(BlueprintType)
struct FACComboSequence
{
	GENERATED_BODY()

	/** 이 콤보를 구성하는 몽타주. 배열 순서대로 한 스텝씩 재생된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<TObjectPtr<UAnimMontage>> Montages;

	/** 콤보 선택 가중치. 값이 클수록 자주 뽑힌다. 전체 합 대비 비율로만 작동하므로 합을 100으로 맞추면 그대로 %가 된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	/** Weight 비율로 계산된 실제 선택 확률. 표시 전용이며 값을 바꾸면 자동으로 갱신된다 */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Combo")
	FString Chance;
};

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

	/**
	 * @brief MontageSelectionMode에 따라 AttackMontages에서 재생할 몽타주를 선택해 반환한다.
	 *
	 * Random이면 매번 무작위로, Sequential이면 배열 순서대로 하나씩 진행하며 끝에 도달하면 처음으로 돌아간다.
	 * @return 선택된 몽타주. AttackMontages가 비어 있으면 nullptr.
	 * @note 어빌리티가 InstancedPerActor이므로 Sequential의 진행 위치는 활성화 사이에도 유지된다.
	 */
	virtual UAnimMontage* SelectAttackMontage() override;

#if WITH_EDITOR
	/** 값이 편집되면 각 콤보의 Chance 표시를 다시 계산한다 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Chance는 Transient라 저장되지 않으므로, 애셋을 열었을 때 바로 보이도록 로드 시 채운다 */
	virtual void PostLoad() override;
#endif

protected:
	/** ComboSequence 모드에서는 ComboSequences에 재생 가능한 콤보가 있는지로 판단한다 (AttackMontages는 비어 있어도 된다). */
	virtual bool HasAnyAttackMontage() const override;

	/**
	 * @brief 쿨다운 적용 시점을 콤보 단위로 미룬다.
	 *
	 * ComboSequence 모드는 스텝마다 어빌리티가 재활성화되므로, 활성화마다 쿨다운을 걸면
	 * 다음 스텝 진입이 막혀 콤보가 1타에서 끊긴다. 그래서 여기서는 적용하지 않고
	 * 콤보 한 벌의 마지막 스텝을 꺼낼 때 SelectComboSequenceMontage가 직접 적용한다.
	 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** 재생할 몽타주를 고르는 방식 — 무작위/순차는 AttackMontages를, 콤보는 ComboSequences를 사용한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Montage", meta = (AllowPrivateAccess = "true"))
	EACAttackMontageSelectionMode MontageSelectionMode = EACAttackMontageSelectionMode::Random;

	/** ComboSequence 모드에서 사용할 콤보 목록. 한 벌을 Weight 비율로 뽑아 순서대로 재생한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Montage", meta = (AllowPrivateAccess = "true", TitleProperty = "{Chance}", EditCondition = "MontageSelectionMode == EACAttackMontageSelectionMode::ComboSequence", EditConditionHides))
	TArray<FACComboSequence> ComboSequences;

	/**
	 * ComboSequence 모드에서 "아직 남은 스텝이 있음"을 나타내는 태그.
	 * 마지막 스텝을 꺼내는 순간 해제되므로, StateTree의 콤보 계속 상태가 이 태그로 진입 여부를 판단할 수 있다.
	 * 비워두면 태그를 관리하지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Montage", meta = (AllowPrivateAccess = "true", Categories = "Enemy.Status", EditCondition = "MontageSelectionMode == EACAttackMontageSelectionMode::ComboSequence", EditConditionHides))
	FGameplayTag ComboInProgressTag;

	/** 마지막 콤보 스텝 이후 이 시간(초)이 지나면 진행 중이던 콤보를 버리고 새 콤보를 뽑는다. 0이면 시간으로 끊지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Montage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s", EditCondition = "MontageSelectionMode == EACAttackMontageSelectionMode::ComboSequence", EditConditionHides))
	float ComboResetTime = 2.f;

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
	 * Phase2 체간 데미지 배율 커브.
	 * 기존 PostureDamage에 이 배율을 곱해 Phase2 강화 체간 데미지를 적용합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2", meta = (AllowPrivateAccess = "true"))
	FScalableFloat PostureDamageMultiplierCurve;

	/**
	 * @brief ComboSequence 모드에서 이번 스텝에 재생할 몽타주를 반환한다.
	 *
	 * 진행 중인 콤보가 없거나 마지막 스텝 이후 ComboResetTime을 초과했으면 새 콤보를 뽑고,
	 * 그렇지 않으면 진행 중인 콤보의 다음 스텝으로 이어간다. 마지막 스텝을 넘기면 다음 호출에서 새 콤보를 뽑는다.
	 * @return 재생할 몽타주. 유효한 콤보가 없으면 nullptr.
	 */
	UAnimMontage* SelectComboSequenceMontage();

	/** ComboSequences에서 Weight 비율로 콤보 하나를 뽑아 인덱스를 반환한다. 유효한 콤보가 없으면 INDEX_NONE */
	int32 PickComboSequenceIndex() const;

	/** ComboInProgressTag를 남은 스텝 유무에 맞춰 켜고 끈다. 태그가 비어 있으면 아무것도 하지 않는다 */
	void UpdateComboInProgressTag(bool bInProgress);

#if WITH_EDITOR
	/** 각 콤보의 Weight 비율을 백분율로 환산해 Chance 문자열에 채운다 (에디터 표시 전용) */
	void RefreshComboChanceDisplay();
#endif

	// Sequential 모드에서 다음에 재생할 AttackMontages 인덱스. InstancedPerActor라 활성화 사이에도 값이 유지된다.
	int32 NextSequentialMontageIndex = 0;

	// 진행 중인 콤보의 ComboSequences 인덱스. INDEX_NONE이면 다음 재생 때 새로 뽑는다
	int32 ActiveComboIndex = INDEX_NONE;

	// 진행 중인 콤보에서 다음에 재생할 스텝 인덱스
	int32 NextComboStepIndex = 0;

	// 마지막으로 콤보 스텝을 재생한 시각(초). ComboResetTime 판정에 사용한다
	float LastComboStepTime = 0.f;
};
