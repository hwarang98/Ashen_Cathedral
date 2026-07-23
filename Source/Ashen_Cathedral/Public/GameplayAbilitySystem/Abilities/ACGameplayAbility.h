// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Enums/ACEnums.h"
#include "ACGameplayAbility.generated.h"

class AACEnemyCharacter;
class AACPlayerCharacter;
class UPawnCombatComponent;
class UACAbilitySystemComponent;
class AACCharacterBase;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

protected:
	#pragma region Native Overrides
	/** 어빌리티가 ASC에 부여될 때 자동으로 호출 */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	#pragma endregion

	#pragma region Cooldown
	/** CanActivateAbility의 쿨다운 검사가 이 태그들을 확인한다. CooldownIdentifierTags가 비어 있으면 쿨다운 없음으로 처리한다. */
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	/** 쿨다운으로 적용할 GE. 어빌리티별 에셋 없이 공용 다이나믹 쿨다운 GE를 사용한다. */
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/**
	 * @brief 쿨다운 GE에 지속시간(SetByCaller)과 식별 태그(DynamicGrantedTags)를 주입해 적용한다.
	 *
	 * @note CooldownIdentifierTags가 비어 있거나 CooldownDurationSeconds가 0 이하면 아무것도 하지 않는다.
	 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 쿨다운 지속 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "ACAbility|Cooldown", meta = (ClampMin = "0.0"))
	float CooldownDurationSeconds = 0.f;

	// 쿨다운 중임을 식별할 고유 태그. 어빌리티마다 다르게 지정하면 공격별 독립 쿨다운이 된다
	UPROPERTY(EditDefaultsOnly, Category = "ACAbility|Cooldown", meta = (Categories = "Cooldown"))
	FGameplayTagContainer CooldownIdentifierTags;
	#pragma endregion

	#pragma region Policy
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACAbility")
	EACAbilityActivationPolicy AbilityActivationPolicy = EACAbilityActivationPolicy::OnTriggered;
	#pragma endregion

	/** 액터 정보에서 UACAbilitySystemComponent를 캐스팅하여 반환 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Ability|Helpers")
	UACAbilitySystemComponent* GetACAbilitySystemComponentFromActorInfo() const;

	/** 액터 정보에서 AACCharacterBase를 캐스팅하여 반환 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Ability|Helpers")
	AACCharacterBase* GetACCharacterFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Ability|Helpers")
	AACPlayerCharacter* GetACPlayerFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Ability|Helpers")
	AACEnemyCharacter* GetACEnemyFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Helpers")
	UPawnCombatComponent* GetPawnCombatComponentFromActorInfo() const;
};