// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ACCalculation_DamageTaken.generated.h"

/**
 * @class UACCalculation_DamageTaken
 * @brief 피해량 계산을 처리하는 클래스이다.
 * UACCalculation_DamageTaken 클래스는 피해량 계산을 처리하기 위한 수행 계산(Execution Calculation)을 정의한다.
 *
 * 게임플레이 효과 시스템에서 실행되며, 주어진 입력값에 따라 최종 피해량을 계산하도록 설계한다.
 * 피해량 계산의 커스터마이징 및 확장을 위해 사용한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACCalculation_DamageTaken : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	/**
	 * UACCalculation_DamageTaken 객체를 초기화하며, 피해량 계산에서 기본 설정을 수행한다.
	 * 게임플레이 효과 시스템에서 올바르게 작동하도록 초기 상태를 정의한다.
	 */
	UACCalculation_DamageTaken();

	/**
	 * @brief 커스텀 실행 로직을 처리하는 메서드이다.
	 *
	 * 이 메서드는 게임플레이 효과 시스템에서 호출되며, 피해량 계산에 필요한 실행 로직을 구현한다.
	 * 입력 매개변수를 기반으로 최종 출력값을 결정하며, 특정 조건에 따라 피해량과 관련된 계산을 적용한다.
	 *
	 * @param ExecutionParams 게임플레이 효과 실행과 관련된 입력 매개변수를 포함한다.
	 * 이 매개변수에는 실행 컨텍스트, 대상 정보 등이 포함된다.
	 *
	 * @param OutExecutionOutput 실행 결과를 저장할 출력 객체이다.
	 * 피해량 계산 결과와 변경된 값을 이 객체에 기록하여 게임플레이에 반영한다.
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};