// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "Manager/PrismatiscapeManager.h"
#include "PrismatiscapeSettings.generated.h"

/**
 * 
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Prismatiscape"))
class PRISMATISCAPE_API UPrismatiscapeSettings : public UDeveloperSettingsBackedByCVars
{
GENERATED_BODY()

public:
UPrismatiscapeSettings();

static UPrismatiscapeSettings* Get() { return GetMutableDefault<UPrismatiscapeSettings>(); }

UFUNCTION(BlueprintPure, Category="Prismatiscape")
static UPrismatiscapeSettings* GetPrismatiscapeSettings() { return Get(); }

/** Gets the settings container name for the settings, either Project or Editor */
virtual FName GetContainerName() const { return "Project"; }
/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
virtual FName GetCategoryName() const { return "Plugins"; }
/** The unique name for your section of settings, uses the class's FName. */
virtual FName GetSectionName() const { return "Prismatiscape"; }

UPROPERTY(EditAnywhere, config, Category = "Default")
TSubclassOf<class APrismatiscapeManager> PrismatiscapeManagerClass;

void SetPrismatiscapeManagerClass(const TSubclassOf<class APrismatiscapeManager>& InPrismatiscapeManagerClass) { PrismatiscapeManagerClass = InPrismatiscapeManagerClass; }

UFUNCTION(BlueprintPure, Category="Prismatiscape")
bool GetDrawDebugShapes() const;


protected:
friend class UPrismatiscapeWorldSubsystem;

/* static but weak ptr used for getting the manager while inside the editor */
TWeakObjectPtr<class APrismatiscapeManager> PrismatiscapeManager;
};