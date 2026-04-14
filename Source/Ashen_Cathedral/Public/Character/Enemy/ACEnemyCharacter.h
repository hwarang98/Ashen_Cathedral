// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "Interfaces/PawnDeathInterface.h"
#include "ACEnemyCharacter.generated.h"

class UWidgetComponent;
class UEnemyUIComponent;

UCLASS()
class ASHEN_CATHEDRAL_API AACEnemyCharacter : public AACCharacterBase, public IPawnDeathInterface
{
	GENERATED_BODY()

public:
	AACEnemyCharacter();

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	virtual UEnemyUIComponent* GetEnemyUIComponent() const override;
	virtual void OnDeath() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyUIComponent> EnemyUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> EnemyHealthWidgetComponent;

	// void InitEnemyStartUpData();
};