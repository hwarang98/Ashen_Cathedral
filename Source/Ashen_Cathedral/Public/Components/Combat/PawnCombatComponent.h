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
	UPawnCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * @brief 무기 콜리전이 대상에 적중했을 때 호출된다.
	 *
	 * @param HitActor  적중된 대상 액터
	 * @param HitResult 무기가 확정한 실제 충돌 정보. 공격자 ASC의 EffectContext에 실려 GameplayCue까지 전달된다.
	 */
	virtual void OnHitTargetActor(AActor* HitActor, const FHitResult& HitResult);
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractingActor);
	virtual void OnHitTargetActorImpl(AActor* HitActor, const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	AACWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTagToResister, AACWeaponBase* InWeaponToResister, bool bResisterAsEquippedWeapon = false);

	/**
	 * @brief 등록된 무기 하나를 목록에서 제거한다. 무기 액터를 파괴하지는 않는다.
	 *
	 * 적중 델리게이트 바인딩을 해제하고, 해당 태그가 현재 장착 태그와 같을 때만 장착 태그를 비운다.
	 * 소켓/애님/입력 해제는 이 함수를 호출하기 전에 해제 경로에서 이미 끝나 있어야 한다.
	 *
	 * @param InWeaponTagToUnregister 제거할 무기의 등록 태그
	 * @return 실제로 제거했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	bool UnregisterWeapon(FGameplayTag InWeaponTagToUnregister);

	// 현재 등록된 모든 무기의 태그 목록. 등록 해제를 순회할 때 사용한다
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|Combat")
	TArray<FGameplayTag> GetCarriedWeaponTags() const;

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	virtual void SetCurrentEquippedWeaponTag(const FGameplayTag& NewWeaponTag);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	AACWeaponBase* GetCharacterCurrentEquippedWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	AACCharacterBase* GetOwnerCharacter() const;

	/** 현재 장착 무기의 기본 데미지를 반환. 자식 클래스에서 무기 데이터에 맞게 override한다. */
	virtual float GetCurrentWeaponBaseDamage() const;
	/** 현재 장착 무기의 약공격 체간 데미지를 반환. */
	virtual float GetCurrentWeaponLightAttackPostureDamage() const;
	/** 현재 장착 무기의 강공격 체간 데미지를 반환. */
	virtual float GetCurrentWeaponHeavyAttackPostureDamage() const;
	/** 현재 장착 무기의 카운터 체간 데미지를 반환. */
	virtual float GetCurrentWeaponCounterAttackPostureDamage() const;
	/** 현재 장착 무기의 공격 속도(몽타주 재생 배율)를 반환. */
	virtual float GetCurrentWeaponAttackSpeed() const;

	UPROPERTY(BlueprintReadWrite, Category = "Ashen Cathdral|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	#pragma region Collision
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|Combat")
	void ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType);

	/** true면 무기 콜리전이 활성화된 동안 콜리전 박스를 화면에 표시한다 (히트박스 타이밍 디버깅용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ashen Cathdral|Combat")
	bool bDebugShowWeaponCollision = false;
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

	// 장착이 확정되는 순간 무기 메시에 EquipNiagaraSystem을 붙여 재생한다. 이펙트가 없으면 아무것도 하지 않는다
	void PlayEquipVFX(const AACWeaponBase* InWeapon, const UACDataAsset_WeaponData* InWeaponData) const;
};