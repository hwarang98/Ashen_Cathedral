// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/PawnCombatInterface.h"
#include "ACCharacterBase.generated.h"

class UACDataAsset_StartupDataBase;
class UACAttributeSet;
class UACAbilitySystemComponent;
class UMotionWarpingComponent;

UCLASS(Abstract)
class ASHEN_CATHEDRAL_API AACCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPawnCombatInterface
{
	GENERATED_BODY()

public:
	AACCharacterBase();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;

	FORCEINLINE UACAbilitySystemComponent* GetACAbilitySystemComponent() const { return ACAbilitySystemComponent; }
	FORCEINLINE UACAttributeSet* GetACAttributeSet() const { return ACAttributeSet; }
	FORCEINLINE TSoftObjectPtr<UACDataAsset_StartupDataBase> GetCharacterStartUpData() const { return CharacterStartUpData; }

protected:
	#pragma region GAS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UACAbilitySystemComponent> ACAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UACAttributeSet> ACAttributeSet;
	#pragma endregion

	#pragma region DataAssets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData | DataAsset")
	TSoftObjectPtr<UACDataAsset_StartupDataBase> CharacterStartUpData;
	#pragma endregion

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionWarping")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
};