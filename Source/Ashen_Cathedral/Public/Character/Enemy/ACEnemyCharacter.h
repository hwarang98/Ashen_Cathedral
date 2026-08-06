// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Character/ACCharacterBase.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "Interfaces/PawnDeathInterface.h"
#include "ACEnemyCharacter.generated.h"

class UWidgetComponent;
class UEnemyUIComponent;
class UACDataAsset_BossReward;

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

	FORCEINLINE UACDataAsset_BossReward* GetBossRewardData() const { return BossRewardData; }
	FORCEINLINE FGameplayTag GetBossIdentityTag() const { return BossIdentityTag; }

private:
	// 보스 개체 여부 — true면 BeginPlay에서 플레이어의 RewardCardComponent에 자동 등록되어 사망 시 카드 보상을 트리거한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RewardCard", meta = (AllowPrivateAccess = "true"))
	bool bIsBoss = false;

	// 이 보스가 어떤 보스인지 식별하는 태그(예: Enemy.Boss.Ordan). bIsBoss가 true일 때 BeginPlay에서 ASC에 Loose 태그로 부여된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (AllowPrivateAccess = "true", Categories = "Enemy.Boss", EditCondition = "bIsBoss", EditConditionHides))
	FGameplayTag BossIdentityTag;

	// 이 보스를 처치했을 때 지급할 성흔 조각 보상 정의. 사망 연출 완료 시 AACGameMode가 조회해 MetaProgressionSubsystem에 전달한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MetaProgression", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UACDataAsset_BossReward> BossRewardData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyUIComponent> EnemyUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> EnemyHealthWidgetComponent;

	// void InitEnemyStartUpData();
};