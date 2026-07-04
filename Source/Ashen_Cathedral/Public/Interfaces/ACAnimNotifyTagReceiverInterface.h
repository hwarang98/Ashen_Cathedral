// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "ACAnimNotifyTagReceiverInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UACAnimNotifyTagReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * ANS_AddGameplayTag가 몽타주 구간 동안 부여/제거하는 GameplayTag를 전달받는 인터페이스
 */
class ASHEN_CATHEDRAL_API IACAnimNotifyTagReceiverInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, Category = "AnimNotify|GameplayTag")
	void OnAnimNotifyAddGameplayTags(const FGameplayTagContainer& GameplayTags);
	virtual void OnAnimNotifyAddGameplayTags_Implementation(const FGameplayTagContainer& GameplayTags)
	{
	}

	UFUNCTION(BlueprintNativeEvent, Category = "AnimNotify|GameplayTag")
	void OnAnimNotifyRemoveGameplayTags(const FGameplayTagContainer& GameplayTags);
	virtual void OnAnimNotifyRemoveGameplayTags_Implementation(const FGameplayTagContainer& GameplayTags)
	{
	}
};