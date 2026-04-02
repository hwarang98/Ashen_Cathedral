// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "ACEnemyGameplayAbility_HitReact.generated.h"

/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyGameplayAbility_HitReact : public UACEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UACEnemyGameplayAbility_HitReact();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> FrontHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> LeftHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> RightHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> BackHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> UnderAttackEffect;

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnMontageCancelled();
};