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
 * @brief Pawn의 전투를 관리하는 컴포넌트.
 *
 * 무기 등록 및 장착, 적 타격 처리, 소유 Pawn과의 상호작용 등의 기능을 제공한다.
 * 게임플레이 내에서 Pawn의 전투 관련 로직을 캡슐화한다.
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

	UPROPERTY(BlueprintReadWrite, Category = "Ashen Cathdral|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

protected:
	// 한 번의 공격 동안 이미 맞은 액터들을 기록하는 배열
	UPROPERTY()
	TArray<TObjectPtr<AActor>> OverlappedActors;

private:
	UPROPERTY()
	TArray<FWeaponEntry> CharacterCarriedWeaponList;

	void HandleEquipEffects(const FGameplayTag& NewWeaponTag, const FGameplayTag& OldWeaponTag);

	void PreloadSkillParticles(const UACDataAsset_WeaponData* WeaponData);
};