// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/UI/PawnUIComponent.h"
#include "EnemyUIComponent.generated.h"

class UACWidgetBase;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UEnemyUIComponent : public UPawnUIComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void RegisterEnemyDrawnWidget(UACWidgetBase* InWidgetToRegister);

	UFUNCTION(BlueprintCallable)
	void RemoveEnemyDrawnWidgetsIfAny();

	/**
	 * @brief 등록된 적 UI 위젯을 일시적으로 숨기거나 되돌린다.
	 *
	 * @param bVisible false면 숨기고, true면 다시 표시한다
	 * @note 조우 컷신처럼 연출 동안 UI를 걷어내야 할 때 사용한다. 제거(RemoveEnemyDrawnWidgetsIfAny)와 달리
	 *       위젯을 파괴하지 않으므로 연출이 끝나면 그대로 복구된다.
	 */
	UFUNCTION(BlueprintCallable)
	void SetEnemyWidgetsVisible(bool bVisible);

private:
	UPROPERTY()
	TArray<UACWidgetBase*> EnemyDrawnWidgets;

	// 현재 적 UI가 숨김 상태인지 여부. 나중에 등록되는 위젯에도 같은 상태를 적용하기 위해 유지한다
	bool bWidgetsHidden = false;
};