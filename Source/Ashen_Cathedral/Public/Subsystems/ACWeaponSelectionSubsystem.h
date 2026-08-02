// 로비에서 고른 무기 데이터를 레벨 전환 너머로 유지하는 GameInstanceSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ACWeaponSelectionSubsystem.generated.h"

class UACDataAsset_WeaponData;

// 선택 무기가 바뀔 때 브로드캐스트된다. 로비 선택대가 표시 상태를 갱신하는 데 사용한다
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectedWeaponChanged, UACDataAsset_WeaponData*, OldWeaponData, UACDataAsset_WeaponData*, NewWeaponData);

/**
 * @brief 플레이어가 선택한 무기 데이터를 보관하는 Subsystem.
 *
 * GameInstance 생명주기를 따르므로 로비<->보스 아레나 레벨 전환(OpenLevel)에도 선택이 유지된다.
 * GameInstance 클래스와 무관하게 부착되므로 UGT 템플릿의 GameInstance를 써도 그대로 동작한다.
 * 상태만 보관하며 실제 스폰/장착 오케스트레이션은 UPlayerCombatComponent가 담당한다.
 * SaveGame에 저장하지 않으므로 게임을 재시작하면 선택은 초기화된다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACWeaponSelectionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 선택 무기를 확정하고 변경을 브로드캐스트한다.
	 * 같은 데이터가 전달되면 아무것도 하지 않는다.
	 * 실제 스폰/장착이 성공한 뒤에 호출해야 잘못된 선택이 확정되지 않는다.
	 * @param InWeaponData 확정할 무기 데이터
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponSelection")
	void SetSelectedWeaponData(UACDataAsset_WeaponData* InWeaponData);

	UFUNCTION(BlueprintPure, Category = "WeaponSelection")
	FORCEINLINE UACDataAsset_WeaponData* GetSelectedWeaponData() const { return SelectedWeaponData; }

	UFUNCTION(BlueprintPure, Category = "WeaponSelection")
	FORCEINLINE bool HasSelectedWeaponData() const { return SelectedWeaponData != nullptr; }

	// 선택을 비우고 변경을 브로드캐스트한다
	UFUNCTION(BlueprintCallable, Category = "WeaponSelection")
	void ClearSelectedWeaponData();

	/**
	 * @brief 교체 연출/파이프라인 진행 여부를 기록한다.
	 * 로비 선택대의 연출 시작과 UPlayerCombatComponent의 교체 시작에서 true로, 양쪽 종료 경로에서 false로 세팅된다.
	 * 이 플래그가 true인 동안에는 다른 선택대 상호작용과 전투 시작이 거부된다.
	 * @param bInProgress 진행 중이면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponSelection")
	void SetWeaponChangeInProgress(bool bInProgress);

	UFUNCTION(BlueprintPure, Category = "WeaponSelection")
	FORCEINLINE bool IsWeaponChangeInProgress() const { return bWeaponChangeInProgress; }

	// 다른 교체가 진행 중이 아닐 때만 true
	UFUNCTION(BlueprintPure, Category = "WeaponSelection")
	bool CanChangeWeaponSelection() const;

	// 선택 무기가 바뀔 때마다 브로드캐스트된다
	UPROPERTY(BlueprintAssignable, Category = "WeaponSelection")
	FOnSelectedWeaponChanged OnSelectedWeaponChangedDelegate;

private:
	UPROPERTY()
	TObjectPtr<UACDataAsset_WeaponData> SelectedWeaponData;

	bool bWeaponChangeInProgress = false;
};
