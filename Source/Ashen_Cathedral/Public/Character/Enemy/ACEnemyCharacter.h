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
	// 보스 개체 여부 — true면 BeginPlay에서 플레이어의 RewardCardComponent에 자동 등록되어 사망 시 카드 보상을 트리거한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RewardCard", meta = (AllowPrivateAccess = "true"))
	bool bIsBoss = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyUIComponent> EnemyUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> EnemyHealthWidgetComponent;

	// void InitEnemyStartUpData();
};