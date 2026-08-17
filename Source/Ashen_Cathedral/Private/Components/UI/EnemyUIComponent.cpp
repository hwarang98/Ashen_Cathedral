// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/UI/EnemyUIComponent.h"

#include "Widget/ACWidgetBase.h"

void UEnemyUIComponent::RegisterEnemyDrawnWidget(UACWidgetBase* InWidgetToRegister)
{
	if (!InWidgetToRegister)
	{
		return;
	}

	EnemyDrawnWidgets.Add(InWidgetToRegister);

	// 보스 체력바를 그리는 어빌리티는 StartUpData 비동기 로드 뒤에 부여되므로, 조우 전에 숨겨 둔 시점보다
	// 늦게 위젯이 만들어진다. 이미 숨김 상태라면 새로 등록되는 위젯도 함께 감춘다.
	if (bWidgetsHidden)
	{
		InWidgetToRegister->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEnemyUIComponent::SetEnemyWidgetsVisible(bool bVisible)
{
	// 이후에 등록되는 위젯에도 같은 상태를 적용하기 위해 기억해 둔다
	bWidgetsHidden = !bVisible;

	for (UACWidgetBase* DrawnWidget : EnemyDrawnWidgets)
	{
		if (DrawnWidget)
		{
			DrawnWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UEnemyUIComponent::RemoveEnemyDrawnWidgetsIfAny()
{
	if (EnemyDrawnWidgets.IsEmpty())
	{
		return;
	}

	for (UACWidgetBase* DrawnWidget : EnemyDrawnWidgets)
	{
		if (DrawnWidget)
		{
			DrawnWidget->RemoveFromParent();
		}
	}
}