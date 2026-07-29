// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "Structs/ACStructTypes.h"
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

	/**
	 * 강한 공격에 피격됐을 때 방향별 몽타주 대신 재생할 몽타주 목록. 위에서부터 검사해 처음 일치하는 항목을 사용한다.
	 * 비워두면 항상 방향별 몽타주를 재생한다(기존 동작).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|Weight", meta = (AllowPrivateAccess = "true"))
	TArray<FACHitReactWeightMontage> WeightHitReactMontages;

	/** 공격에 실려 온 속성 태그로 대형 히트리액트 몽타주를 찾는다. 해당 항목이 없으면 nullptr */
	UAnimMontage* SelectWeightHitReactMontage(const FGameplayTagContainer& AttackTags) const;

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnMontageCancelled();
};