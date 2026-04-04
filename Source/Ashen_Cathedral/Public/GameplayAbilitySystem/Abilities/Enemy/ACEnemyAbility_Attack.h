// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACEnemyAbility_Attack.generated.h"

/**
 * 적 전용 공격 어빌리티.
 * 콤보 흐름은 Behavior Tree가 제어하므로 HandleComboComplete는 아무것도 하지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_Attack : public UACAbility_Attack
{
	GENERATED_BODY()

public:
	UACEnemyAbility_Attack();

	/** BT가 다음 공격을 결정하므로 콤보 완료 시 아무것도 하지 않는다. */
	virtual void HandleComboComplete() override;

	/** AttackMontages 배열에서 무작위로 몽타주를 선택해 반환한다. */
	virtual UAnimMontage* SelectAttackMontage() override;
};
