// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ACRewardCardSelectionWidget.h"

void UACRewardCardSelectionWidget::SetupCards(const TArray<FACRewardCardDisplayInfo>& Cards)
{
	BP_SetupCards(Cards);
}

void UACRewardCardSelectionWidget::NotifyCardSelected(FName CardID)
{
	if (OnCardSelectedDelegate.IsBound())
	{
		OnCardSelectedDelegate.Execute(CardID);
	}
}
