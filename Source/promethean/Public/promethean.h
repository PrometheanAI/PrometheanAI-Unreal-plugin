// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPromethean, Log, All);

class FprometheanModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FDelegateHandle OnPostWorldCreationDelegateHandle;  // currently unused. for the auto connect vr hotkey function
};
