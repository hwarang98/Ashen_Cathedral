// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Attack.h"

void UACEnemyAbility_Attack::HandleComboComplete()
{
	// BT가 공격 타이밍을 제어하므로 콤보 카운트 리셋을 하지 않는다.
	// HandleComboCancelled는 베이스 클래스(즉시 리셋)를 그대로 사용
}

UAnimMontage* UACEnemyAbility_Attack::SelectAttackMontage()
{
	const int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);
	return AttackMontages[RandomIndex];
}
