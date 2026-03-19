// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Enums/ACEnums.h"
#include "ACGameplayAbility.generated.h"

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

	#pragma region Policy
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CMAbility")
	EACAbilityActivationPolicy AbilityActivationPolicy = EACAbilityActivationPolicy::OnTriggered;
	#pragma endregion

	/** 액터 정보에서 UCMAbilitySystemComponent를 캐스팅하여 반환 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Ability|Helpers")
	UACAbilitySystemComponent* GetACAbilitySystemComponentFromActorInfo() const;

	/** 액터 정보에서 ACMCharacterBase를 캐스팅하여 반환 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Ability|Helpers")
	AACCharacterBase* GetCMCharacterFromActorInfo() const;
};