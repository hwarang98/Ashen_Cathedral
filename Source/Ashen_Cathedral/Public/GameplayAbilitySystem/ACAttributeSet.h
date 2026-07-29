// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ACAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

class IPawnUIInterface;
/**
 * @class UACAttributeSet
 * @brief UACAttributeSet 클래스는 Ashen Cathedral 프로젝트의 캐릭터 속성(Attribute)을 정의하고 관리하는 역할을 수행합니다.
 *
 * 이 클래스는 Unreal Engine의 UAttributeSet를 상속받아 구현되었으며, 게임 플레이 능력 시스템(GAS: Gameplay Ability System)과 연계되어
 * 캐릭터의 여러 속성(health, mana, strength 등)을 효율적으로 관리하거나 업데이트하는 데 사용됩니다.
 *
 * UACAttributeSet은 AACCharacterBase 클래스와 같은 캐릭터 클래스에서 주로 사용되며, 캐릭터의 상태와 능력에 따라
 * 속성값이 동적으로 변경될 수 있습니다.
 *
 * [가스 시스템 연계]
 * - GAS를 통해 적용되는 능력(Abilities)을 활성화하거나 종료할 때 속성 값을 자동으로 계산 및 업데이트합니다.
 * - 예: 캐릭터의 체력이 감소하거나 스킬에 의해 스탯이 변화될 경우, 속성을 변경하고 이를 다른 관련 시스템에 알립니다.
 *
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UACAttributeSet();
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	/** GE_PostureDecay 같은 주기형 Additive Modifier가 BaseValue를 직접 변경할 때도 음수로 드리프트하지 않도록 클램프한다. */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	/** MoveSpeed 변경(GE 적용·제거 모두) 시 CharacterMovement 와 동기화 */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	#pragma region Core - 생명력 & 스태미나
	/** 기본 이동 속도. CharacterMovement 의 MaxWalkSpeed 와 동기화됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, MoveSpeed);

	/** 현재 체력. 0이 되면 사망 처리됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, Health);

	/** 최대 체력. Health의 상한선이 됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, MaxHealth);

	/** 현재 스태미나. 공격·회피·스프린트 등 행동 시 소모됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, Stamina);

	/** 최대 스태미나. Stamina의 상한선이 됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, MaxStamina);

	/** 초당 스태미나 회복량. 전투 템포 및 플레이 감각에 큰 영향을 미칩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, StaminaRegenRate);
	#pragma endregion

	#pragma region Posture - 체간 게이지
	/** 현재 체간(Posture) 누적 수치. 피격 시 증가하며 MaxPosture에 도달하면 체간 붕괴 상태에 진입합니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Posture")
	FGameplayAttributeData Posture;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, Posture);

	/** 최대 체간 게이지. Posture의 상한선이 됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Posture")
	FGameplayAttributeData MaxPosture;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, MaxPosture);

	/** 체간 저항력. 피격 시 Posture 증가량을 감소시킵니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Posture")
	FGameplayAttributeData PostureResistance;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, PostureResistance);

	#pragma endregion

	#pragma region Guard - 가드 게이지
	/** 현재 가드 누적 수치. 공격을 막아낼 때마다 증가하며 MaxGuardGauge에 도달하면 가드 브레이크가 발생합니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Guard")
	FGameplayAttributeData GuardGauge;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, GuardGauge);

	/** 최대 가드 게이지. GuardGauge의 상한선이며, 이 값에 도달하면 가드가 무너집니다. 보상 카드로 증가시킬 수 있습니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Guard")
	FGameplayAttributeData MaxGuardGauge;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, MaxGuardGauge);

	/** 가드 저항력. 막아냈을 때 GuardGauge 증가량을 감소시킵니다. 보상 카드로 증가시킬 수 있습니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Guard")
	FGameplayAttributeData GuardBreakResistance;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, GuardBreakResistance);

	/** 초당 가드 게이지 자연 회복량. 주기형 GE가 이 값을 참조해 게이지를 감소시킵니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Guard")
	FGameplayAttributeData GuardGaugeRegenRate;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, GuardGaugeRegenRate);

	/** 이번에 막아내며 받은 가드 부하. PostGameplayEffectExecute에서 소비 후 초기화되는 메타 Attribute입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Guard")
	FGameplayAttributeData GuardDamageTaken;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, GuardDamageTaken);
	#pragma endregion

	#pragma region Combat - 전투 능력치
	/** 공격력. 기본 데미지 계산의 기준이 되는 값입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, AttackPower);

	/** 방어력. 피격 시 받는 데미지를 감소시키는 값입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData DefensePower;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, DefensePower);

	/** 공격 속도. 공격 애니메이션 재생 속도 및 연속 공격 간격에 영향을 미칩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, AttackSpeed);

	/** 피격 시 받은 데미지. PostGameplayEffectExecute에서 소비 후 초기화되는 메타 Attribute입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData DamageTaken;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, DamageTaken);

	/** 체간 데미지를 받은 수치. PostGameplayEffectExecute에서 소비 후 초기화되는 메타 Attribute입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Posture")
	FGameplayAttributeData PostureDamageTaken;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, PostureDamageTaken);

	/** 스태미나 소비량. PostGameplayEffectExecute에서 소비 후 초기화되는 메타 Attribute. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Combat")
	FGameplayAttributeData StaminaCost;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, StaminaCost);
	#pragma endregion

	#pragma region Burn - 화상 게이지
	/** 현재 화상 누적 수치. 피격 시 증가하며 MaxBurnGauge에 도달하면 DoT가 발동됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Burn")
	FGameplayAttributeData BurnGauge;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, BurnGauge);

	/** 최대 화상 게이지. BurnGauge의 상한선이 됩니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Burn")
	FGameplayAttributeData MaxBurnGauge;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, MaxBurnGauge);

	/** 화상 축적량. PostGameplayEffectExecute에서 소비 후 초기화되는 메타 Attribute입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute|Burn")
	FGameplayAttributeData BurnAccumulation;
	ATTRIBUTE_ACCESSORS(UACAttributeSet, BurnAccumulation);
	#pragma endregion

private:
	void HandlePostureDamage(const FGameplayEffectModCallbackData& Data);
	/** 가드 부하를 GuardGauge에 누적하고, 최대치에 도달하면 가드 브레이크 이벤트를 발송한다. */
	void HandleGuardDamage(const FGameplayEffectModCallbackData& Data);
	void HandleDamageAndTriggerHitReact(const FGameplayEffectModCallbackData& Data);
	void HandleStaminaConsumption(const FGameplayEffectModCallbackData& Data);
	void HandleBurnBuildUp(const FGameplayEffectModCallbackData& Data);
	TWeakInterfacePtr<IPawnUIInterface> CachedPawnUIInterface;
};