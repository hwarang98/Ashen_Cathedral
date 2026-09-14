// Copyright 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FPrismatiscapeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
