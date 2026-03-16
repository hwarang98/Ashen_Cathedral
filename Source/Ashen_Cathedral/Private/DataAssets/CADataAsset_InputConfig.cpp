// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/CADataAsset_InputConfig.h"

#include "Structs/ACStructTypes.h"

UInputAction* UCADataAsset_InputConfig::FindNativeInputActionByTag(const FGameplayTag& InputTag) const
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

UInputAction* UCADataAsset_InputConfig::FindAbilityInputActionByTag(const FGameplayTag& InputTag) const
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
