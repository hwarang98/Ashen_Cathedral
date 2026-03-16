// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ACCharacterBase.generated.h"

class UACAttributeSet;
class UACAbilitySystemComponent;

UCLASS()
class ASHEN_CATHEDRAL_API AACCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AACCharacterBase();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FORCEINLINE UACAbilitySystemComponent* GetACAbilitySystemComponent() const { return ACAbilitySystemComponent; }
	FORCEINLINE UACAttributeSet* GetACAttributeSet() const { return ACAttributeSet; }

protected:
#pragma region GAS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UACAbilitySystemComponent> ACAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UACAttributeSet> ACAttributeSet;
#pragma endregion
};
