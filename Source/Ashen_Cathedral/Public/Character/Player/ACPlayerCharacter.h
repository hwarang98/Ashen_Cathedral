// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Interfaces/InteractableInterface.h"
#include "Structs/ACStructTypes.h"
#include "ACPlayerCharacter.generated.h"

class UPlayerUIComponent;
class UCameraComponent;
class USpringArmComponent;
class UACDataAsset_InputConfig;
class UGameplayCameraComponent;
class UACRewardCardComponent;

// ActionStates가 변경될 때(태그 추가/제거) Broadcast. GameplayCamera Chooser 재평가 트리거용
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionStatesChanged);

UCLASS()
class ASHEN_CATHEDRAL_API AACPlayerCharacter : public AACCharacterBase
{
	GENERATED_BODY()

public:
	AACPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnJumped_Implementation() override;
	virtual UPlayerCombatComponent* GetPawnCombatComponent() const override;
	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	virtual UPlayerUIComponent* GetPlayerUIComponent() const;
	virtual void OnAnimNotifyAddGameplayTags_Implementation(const FGameplayTagContainer& GameplayTags) override;
	virtual void OnAnimNotifyRemoveGameplayTags_Implementation(const FGameplayTagContainer& GameplayTags) override;

	// GameplayCamera Chooser Table에 넘길 계약 구조체를 현재 상태로 채워 반환
	UFUNCTION(BlueprintPure, Category = "Camera")
	FACCameraChooserContext MakeCameraChooserContext() const;
	FORCEINLINE UACRewardCardComponent* GetRewardCardComponent() const { return RewardCardComponent; }

	// ActionStates가 바뀔 때만 Broadcast. 카메라 쪽에서 Bind해두면 매 틱 폴링 없이 Chooser를 재평가할 수 있음
	UPROPERTY(BlueprintAssignable, Category = "Camera")
	FOnActionStatesChanged OnActionStatesChanged;

	/** Light/Heavy 어빌리티가 공유하는 콤보 카운트. SelectAttackMontage()가 이 값을 기준으로 몽타주를 선택한다. */
	int32 SharedComboCount = 0;

	/** Light/Heavy 어빌리티가 공유하는 콤보 리셋 타이머 핸들. 하나만 유지되므로 어빌리티 전환 시 이전 타이머가 자동으로 교체된다. */
	FTimerHandle SharedComboResetTimerHandle;

	/** 마지막 공격 후 이 시간 안에 재입력이 없으면 콤보 카운트를 리셋한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Combo")
	float ComboResetDelay = 2.0f;

	// 상호작용 가능한 액터가 오버랩 범위에 들어왔을 때 호출
	void SetCurrentInteractable(TScriptInterface<IInteractableInterface> InInteractable);

	// 상호작용 가능한 액터가 오버랩 범위를 벗어났을 때 호출. InInteractable이 현재 등록된 대상과 일치할 때만 해제
	void ClearCurrentInteractable(TScriptInterface<IInteractableInterface> InInteractable);

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData | DataAsset", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UACDataAsset_InputConfig> InputConfigDataAsset;

	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera | SpringArm", meta = (AllowPrivateAccess = "true"))
	// TObjectPtr<USpringArmComponent> CameraBoom;
	//
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	// TObjectPtr<UCameraComponent> ViewCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerCombatComponent> PlayerCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerUIComponent> PlayerUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RewardCard", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UACRewardCardComponent> RewardCardComponent;

	// GameplayCamera Chooser Table에서 바인딩해 Player.ActionState.* 태그를 읽는 컨테이너
	UPROPERTY(BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer ActionStates;

	#pragma region Sprint
	/** 이동 입력 해제 시 Sprint 취소 */
	void StopSprint();
	#pragma endregion

	#pragma region Block Strafe
	/** Player_Status_Blocking 태그 추가/제거 시 Strafe 모드를 전환한다 */
	void OnBlockingTagChanged(const FGameplayTag Tag, int32 NewCount);
	#pragma endregion

	#pragma region  Input Callback
	void Input_AbilityInputPressed(const FGameplayTag InInputTag);
	void Input_AbilityInputReleased(const FGameplayTag InInputTag);

	void Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue);
	void Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue);

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	void Input_Interact();
	#pragma endregion

	FVector2D SwitchDirection = FVector2D::ZeroVector;

	// 현재 오버랩 범위 안에 있어 상호작용 입력이 들어오면 호출할 대상
	TScriptInterface<IInteractableInterface> CurrentInteractable;
};