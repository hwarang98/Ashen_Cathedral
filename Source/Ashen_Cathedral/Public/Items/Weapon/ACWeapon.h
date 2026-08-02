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

	// 스폰 시점에 선택된 WeaponData를 주입한다. BP 기본값을 덮어써 액터와 데이터가 어긋나는 것을 막는다
	UFUNCTION(BlueprintCallable)
	void SetWeaponData(UACDataAsset_WeaponData* InWeaponData);

	UFUNCTION(BlueprintCallable)
	void AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InSpecHandles);

	UFUNCTION(BlueprintPure)
	const TArray<FGameplayAbilitySpecHandle>& GetGrantedAbilitySpecHandles() const;

	/**
	 * @brief 이 무기와 함께 부여된 장착 어빌리티의 스펙 핸들을 기록한다.
	 *
	 * 장착 어빌리티는 무기를 손에 쥐기 전에도 입력을 받아야 하므로 DefaultWeaponAbilities와 수명이 다르다.
	 * 무기를 파괴하기 전에 이 핸들을 반드시 ClearAbility 해야 이전 무기의 장착 입력이 ASC에 남지 않는다.
	 *
	 * @param InSpecHandle GiveAbility가 반환한 장착 어빌리티 스펙 핸들
	 */
	UFUNCTION(BlueprintCallable)
	void SetEquipAbilitySpecHandle(const FGameplayAbilitySpecHandle& InSpecHandle);

	UFUNCTION(BlueprintPure)
	const FGameplayAbilitySpecHandle& GetEquipAbilitySpecHandle() const;

	UFUNCTION(BlueprintPure)
	bool HasValidEquipAbilitySpecHandle() const;

	UFUNCTION(BlueprintCallable)
	void ClearEquipAbilitySpecHandle();

private:
	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;

	// 이 무기 전용으로 부여된 장착 어빌리티 스펙 핸들 (WeaponData->EquipAbility로부터 부여됨)
	FGameplayAbilitySpecHandle EquipAbilitySpecHandle;
};