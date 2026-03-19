// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACCharacterBase.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Structs/ACStructTypes.h"
#include "PawnCombatComponent.generated.h"

class UACDataAsset_WeaponData;
class AACWeaponBase;
struct FGameplayTag;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UPawnCombatComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	virtual void OnHitTargetActor(AActor* HitActor);
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractingActor);
	virtual void OnHitTargetActorImpl(AActor* HitActor);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	AACWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTagToResister, AACWeaponBase* InWeaponToResister, bool bResisterAsEquippedWeapon = false);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	void SetCurrentEquippedWeaponTag(const FGameplayTag& NewWeaponTag);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	AACWeaponBase* GetCharacterCurrentEquippedWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	AACCharacterBase* GetOwnerCharacter() const;

protected:
	// 한 번의 공격 동안 이미 맞은 액터들을 기록하는 배열
	UPROPERTY()
	TArray<TObjectPtr<AActor>> OverlappedActors;

	FGameplayTag CurrentEquippedWeaponTag;

private:
	UPROPERTY()
	TArray<FWeaponEntry> CharacterCarriedWeaponList;

	void HandleEquipEffects(const FGameplayTag& NewWeaponTag, const FGameplayTag& OldWeaponTag);

	void PreloadSkillParticles(const UACDataAsset_WeaponData* WeaponData);
};