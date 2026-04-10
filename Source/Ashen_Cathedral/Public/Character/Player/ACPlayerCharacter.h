// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/PlayerCombatComponent.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "ACPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UACDataAsset_InputConfig;
class UGameplayCameraComponent;

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

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData | DataAsset", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UACDataAsset_InputConfig> InputConfigDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera | SpringArm", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> ViewCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerCombatComponent> PlayerCombatComponent;


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
	#pragma endregion

	FVector2D SwitchDirection = FVector2D::ZeroVector;
};