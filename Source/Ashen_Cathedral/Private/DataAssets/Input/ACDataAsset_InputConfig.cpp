// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/Input/ACDataAsset_InputConfig.h"
#include "Structs/ACStructTypes.h"

UInputAction* UACDataAsset_InputConfig::FindNativeInputActionByTag(const FGameplayTag& InputTag) const
{
	for (const FCAInputActionConfig& InputActionConfig : NativeInputActions)
	{
		if (InputActionConfig.InputTag == InputTag && InputActionConfig.InputAction)
		{
			return InputActionConfig.InputAction;
		}
	}

	return nullptr;
}

UInputAction* UACDataAsset_InputConfig::FindAbilityInputActionByTag(const FGameplayTag& InputTag) const
{
	for (const FCAInputActionConfig& InputActionConfig : AbilityInputActions)
	{
		if (InputActionConfig.InputTag == InputTag && InputActionConfig.InputAction)
		{
			return InputActionConfig.InputAction;
		}
	}

	return nullptr;
}