// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ACWidgetBase.generated.h"

class UEnemyUIComponent;
class UPlayerUIComponent;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void InitEnemyCreatedWidget(AActor* OwningEnemyActor);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnOwningPlayerUIComponentInitialized"))
	void BP_OnOwningPlayerUIComponentInitialized(UPlayerUIComponent* OwningPlayerComponent);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnOwningEnemyUIComponentInitialized"))
	void BP_OnOwningEnemyUIComponentInitialized(UEnemyUIComponent* OwningEnemyComponent);
};