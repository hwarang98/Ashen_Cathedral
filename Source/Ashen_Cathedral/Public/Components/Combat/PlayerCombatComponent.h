// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "DataTable/Item/Weapon/ACDataTable_Weapon.h"
#include "GameplayAbilitySpecHandle.h"
#include "PlayerCombatComponent.generated.h"

class AACWeapon;
class UACAbilitySystemComponent;
class UACDataAsset_WeaponData;
class UACWeaponSelectionSubsystem;
struct FAbilityEndedData;

// 무기 교체 파이프라인이 종료될 때 브로드캐스트된다. 성공/실패와 무관하게 교체당 정확히 한 번 호출된다
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSwapFinished, bool, bSuccess);

/**
 *
 */
UCLASS()
class ASHEN_CATHEDRAL_API UPlayerCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	UPlayerCombatComponent();

	// 태그로 소지 중인 무기를 반환
	AACWeapon* GetPlayerCarriedWeaponByTag(FGameplayTag InWeaponTag) const;

	// 현재 장착된 무기를 반환
	AACWeapon* GetPlayerCurrentEquippedWeapon() const;

	// 현재 장착된 무기의 DataAsset을 반환
	const UACDataAsset_WeaponData* GetPlayerCurrentWeaponData() const;

	// 현재 장착 무기의 레벨별 기본 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Damage")
	float GetPlayerCurrentEquippedWeaponDamageAtLevel() const;

	// 현재 장착 무기의 약공격 체간 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Posture")
	float GetPlayerCurrentWeaponLightPostureDamage() const;

	// 현재 장착 무기의 강공격 체간 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Posture")
	float GetPlayerCurrentWeaponHeavyPostureDamage() const;

	// 현재 장착 무기의 카운터 체간 데미지를 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon|Posture")
	float GetPlayerCurrentWeaponCounterPostureDamage() const;

	// 현재 장착 무기의 타입을 반환
	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon")
	EACWeaponType GetPlayerCurrentWeaponType() const;

	virtual float GetCurrentWeaponBaseDamage() const override;
	virtual float GetCurrentWeaponLightAttackPostureDamage() const override;
	virtual float GetCurrentWeaponHeavyAttackPostureDamage() const override;
	virtual float GetCurrentWeaponCounterAttackPostureDamage() const override;
	virtual float GetCurrentWeaponAttackSpeed() const override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief 장착 태그 전이를 감시해 해제 완료 시점을 파이프라인에 알린다.
	 *
	 * 해제 어빌리티는 자기 스펙을 ClearAbility 하므로 ASC의 OnAbilityEnded가 보장되지 않는다.
	 * 대신 태그가 무효로 바뀌는 순간을 해제 완료 신호로 쓴다.
	 *
	 * @param NewWeaponTag 새로 장착할 무기 태그. 해제라면 무효 태그
	 * @note 이 시점은 GAS 어빌리티 리스트 락 안이므로 후속 작업은 반드시 다음 틱으로 미룬다.
	 */
	virtual void SetCurrentEquippedWeaponTag(const FGameplayTag& NewWeaponTag) override;

	/**
	 * @brief 현재 무기를 해제·파괴하고 InNewWeaponData의 무기로 교체하는 비동기 파이프라인을 시작한다.
	 *
	 * 무장 상태가 아니면 해제 단계를 건너뛰고 곧바로 스폰·장착으로 진행한다.
	 * 성공/실패와 무관하게 종료 시 OnWeaponSwapFinishedDelegate가 브로드캐스트된다.
	 *
	 * @param InNewWeaponData 교체할 무기 데이터. WeaponClassToSpawn / WeaponTypeTag / EquipAbility가 모두 유효해야 한다
	 * @param bPlayUnequipMontage false면 기존 무기의 해제 어빌리티를 실행하지 않고 즉시 해제한다
	 * @param bPlayEquipMontage false면 장착 어빌리티를 실행하지 않고 즉시 장착한다
	 * @return 파이프라인을 실제로 시작했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathedral|Weapon Swap")
	bool RequestWeaponSwap(UACDataAsset_WeaponData* InNewWeaponData, bool bPlayUnequipMontage = true, bool bPlayEquipMontage = true);

	/**
	 * @brief WeaponSelectionSubsystem에 보관된 선택 무기를 다음 틱에 스폰·장착한다.
	 * 전투 레벨 진입 시 선택 무기 스폰 어빌리티가 호출한다.
	 * @return 선택된 무기가 있어 예약했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathedral|Weapon Swap")
	bool RequestSpawnSelectedWeapon();

	/**
	 * @brief 현재 장착 무기의 장착 상태를 해제한다 (어빌리티 회수, 태그 정리, UI 갱신).
	 * 해제 어빌리티의 실제 처리 본문이자, 몽타주가 중단됐을 때 파이프라인이 상태를 강제 정규화하는 경로다.
	 */
	void ApplyUnequipEffects();

	/**
	 * @brief 무기를 실제로 장착한다 (DefaultWeaponAbilities 지급, 소켓 부착, 태그 적용, UI 갱신).
	 * 장착 어빌리티의 몽타주 노티파이와 즉시 교체 경로가 같은 이 함수를 사용한다.
	 * 이미 같은 무기가 장착돼 있으면 중복 지급을 막기 위해 아무것도 하지 않는다.
	 * @param InWeapon 장착할 무기. WeaponData와 그 WeaponTypeTag가 유효해야 한다
	 */
	void ApplyEquipEffects(AACWeapon* InWeapon);

	UFUNCTION(BlueprintPure, Category = "Ashen Cathedral|Weapon Swap")
	FORCEINLINE bool IsWeaponSwapInProgress() const { return SwapPhase != EACWeaponSwapPhase::Idle; }

	UPROPERTY(BlueprintAssignable, Category = "Ashen Cathedral|Weapon Swap")
	FOnWeaponSwapFinished OnWeaponSwapFinishedDelegate;

protected:
	// 각 대기 단계가 이 시간을 넘기면 워치독이 강제로 다음 단계로 진행시킨다. 교체가 영구히 멈추는 것을 막는 안전장치
	UPROPERTY(EditDefaultsOnly, Category = "Ashen Cathedral|Weapon Swap", meta = (ClampMin = "1.0"))
	float SwapPhaseTimeoutSeconds = 6.f;

private:
	// 현재 장착 무기의 DataTable Row를 반환
	const FACWeaponStatRow* GetCurrentWeaponStatRow() const;

	#pragma region Weapon Swap
	// 기존 무기의 해제 어빌리티를 클래스로 찾아 활성화한다. 찾지 못하면 곧바로 강제 해제 경로로 넘어간다
	void BeginUnequipPhase();

	// 해제 완료 후 다음 틱에 이어질 처리를 예약한다. 여러 번 호출해도 한 번만 예약된다
	void ScheduleSwapContinuation();

	void HandleSwapContinuation();

	// 등록 무기를 모두 정리한 뒤 새 무기를 스폰·등록·부착하고 장착 어빌리티를 부여·활성화한다
	void BeginSpawnAndEquipPhase();

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	void OnEquipPhaseFinished();

	void AbortSwap(const FString& InReason);

	// bWeaponChangeInProgress가 해제되는 유일한 지점
	void FinishSwap(bool bSuccess);

	// 등록된 모든 무기의 어빌리티를 회수하고 등록 해제 후 파괴한다. 실제 플레이어 무기는 항상 최대 1개라는 불변식을 강제한다
	void DestroyAllRegisteredWeapons();

	// 무기 하나에 매달린 DefaultWeaponAbilities와 장착 어빌리티 스펙을 모두 회수한다. 파괴 전에 반드시 호출한다
	void ClearWeaponAbilitySpecs(AACWeapon* InWeapon);

	void StartPhaseWatchdog();
	void ClearPhaseWatchdog();
	void HandlePhaseWatchdogExpired();

	void HandleDeferredSpawnSelected();

	UACWeaponSelectionSubsystem* GetWeaponSelectionSubsystem() const;
	UACAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;

	// ASC의 어빌리티 종료 델리게이트에 한 번만 바인드한다
	void EnsureAbilityEndedBinding();

	EACWeaponSwapPhase SwapPhase = EACWeaponSwapPhase::Idle;

	// 이번 교체에서 해제/장착 어빌리티(몽타주)를 실행할지 여부
	bool bPlayUnequipMontageRequested = true;
	bool bPlayEquipMontageRequested = true;

	UPROPERTY()
	TObjectPtr<UACDataAsset_WeaponData> PendingWeaponData;

	UPROPERTY()
	TObjectPtr<AACWeapon> PendingWeapon;

	FGameplayAbilitySpecHandle PendingEquipHandle;

	FDelegateHandle AbilityEndedDelegateHandle;

	FTimerHandle SwapContinuationTimerHandle;

	FTimerHandle SwapWatchdogTimerHandle;
	#pragma endregion
};