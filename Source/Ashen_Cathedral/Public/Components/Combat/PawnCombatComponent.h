// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACCharacterBase.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Enums/ACEnums.h"
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

	/** 현재 장착 무기의 기본 데미지를 반환. 자식 클래스에서 무기 데이터에 맞게 override한다. */
	virtual float GetCurrentWeaponBaseDamage() const;
	/** 현재 장착 무기의 강공격 그로기 데미지를 반환. */
	virtual float GetCurrentWeaponHeavyAttackGroggyDamage() const;
	/** 현재 장착 무기의 카운터 그로기 데미지를 반환. */
	virtual float GetCurrentWeaponCounterAttackGroggyDamage() const;
	/** 현재 장착 무기의 공격 속도(몽타주 재생 배율)를 반환. */
	virtual float GetCurrentWeaponAttackSpeed() const;

	UPROPERTY(BlueprintReadWrite, Category = "Ashen Cathdral|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	#pragma region Collision
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	void ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType);
	#pragma endregion

protected:
	// 한 번의 공격 동안 이미 맞은 액터들을 기록하는 배열
	UPROPERTY()
	TArray<TObjectPtr<AActor>> OverlappedActors;

	#pragma region Internal
	virtual void HandleToggleCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType);
	#pragma endregion

private:
	#pragma region Weapon Data
	UPROPERTY()
	TArray<FWeaponEntry> CharacterCarriedWeaponList;
	#pragma endregion

	void HandleEquipEffects(const FGameplayTag& NewWeaponTag, const FGameplayTag& OldWeaponTag);

	void PreloadSkillParticles(const UACDataAsset_WeaponData* WeaponData);
};