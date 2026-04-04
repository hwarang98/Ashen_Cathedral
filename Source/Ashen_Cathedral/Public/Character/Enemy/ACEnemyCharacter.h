// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "ACEnemyCharacter.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API AACEnemyCharacter : public AACCharacterBase
{
	GENERATED_BODY()

public:
	AACEnemyCharacter();

	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	virtual void PossessedBy(AController* NewController) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;

	void InitEnemyStartUpData();
};