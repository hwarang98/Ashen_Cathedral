// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACCharacterBase.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "ACPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UCADataAsset_InputConfig;
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

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData | DataAsset", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCADataAsset_InputConfig> InputConfigDataAsset;

	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	// TObjectPtr<UGameplayCameraComponent> GameplayCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera | SpringArm", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> ViewCamera;

	#pragma region Sprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement | Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement | Sprint", meta = (AllowPrivateAccess = "true"))
	float RunSpeed = 400.f;

	// 이 속도(XY 평면) 이하로 떨어지면 Sprint 자동 해제
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement | Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintStopThreshold = 50.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement | Sprint", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	void StartSprint();
	void StopSprint();
	#pragma endregion

	#pragma region  Input Callback
	void Input_AbilityInputPressed(const FGameplayTag InInputTag);
	void Input_AbilityInputReleased(const FGameplayTag InInputTag);
	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	#pragma endregion
};