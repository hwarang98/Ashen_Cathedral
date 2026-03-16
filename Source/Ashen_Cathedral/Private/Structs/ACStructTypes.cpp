// Fill out your copyright notice in the Description page of Project Settings.


#include "Structs/ACStructTypes.h"

bool FCAInputActionConfig::IsValid() const
{
	return InputTag.IsValid() && InputAction;
}
