// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ACBossClearWidget.h"

void UACBossClearWidget::NotifyNextRequested()
{
	if (OnNextRequestedDelegate.IsBound())
	{
		OnNextRequestedDelegate.Execute();
	}
}
