// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapon/ACWeaponBase.h"
#include "ACWeapon.generated.h"

struct FGameplayAbilitySpecHandle;
class UACDataAsset_WeaponData;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACWeapon : public AACWeaponBase
{
	GENERATED_BODY()

public:
	// virtual void BeginPlay() override;
	// virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponData")
	TObjectPtr<UACDataAsset_WeaponData> WeaponData;

	UFUNCTION(BlueprintCallable)
	void AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InSpecHandles);

	UFUNCTION(BlueprintPure)
	const TArray<FGameplayAbilitySpecHandle>& GetGrantedAbilitySpecHandles() const;

private:
	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;
};