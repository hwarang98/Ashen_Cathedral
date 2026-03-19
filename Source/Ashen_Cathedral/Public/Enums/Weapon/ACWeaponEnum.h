// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "ACWeaponEnum.generated.h"

UENUM(BlueprintType)
enum class EACWeaponType : uint8
{
	None UMETA(DisplayName = "None"),
	Sword UMETA(DisplayName = "한손검"),
	GreatSword UMETA(DisplayName = "양손검"),
	Spear UMETA(DisplayName = "창"),
	Axe UMETA(DisplayName = "도끼"),
	Dagger UMETA(DisplayName = "단검"),
};