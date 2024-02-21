// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "promethean.h"
#include "PrometheanManager.h"

#include "ViewportWorldInteraction.h"


#define LOCTEXT_NAMESPACE "FprometheanModule"

DEFINE_LOG_CATEGORY(LogPromethean);


void FprometheanModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	LaunchTCP();
	// OnPostWorldCreationDelegateHandle = FWorldDelegates::OnPostWorldCreation.AddStatic(&SubscribeToVRKeyEvents);  // - VR editor mode doesn't create a new world so this is moot
}

void FprometheanModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// FWorldDelegates::OnPostWorldCreation.Remove(OnPostWorldCreationDelegateHandle);
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FprometheanModule, promethean)
