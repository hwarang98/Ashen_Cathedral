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

	// BTTask_ToggleStrafingState에서 ENABLE/DISABLE 노드 간 핸들 공유에 사용
	FActiveGameplayEffectHandle StrafingSpeedEffectHandle;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;
};
