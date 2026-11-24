// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRColocationModule.h"
#include "IOculusXRHMDModule.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusXRHMDModule.h"

DEFINE_LOG_CATEGORY(LogOculusXRColocation);

#define LOCTEXT_NAMESPACE "OculusXRColocation"

//-------------------------------------------------------------------------------------------------
// FOculusXRColocationModule
//-------------------------------------------------------------------------------------------------
void FOculusXRColocationModule::StartupModule()
{
	ColocationXR = MakeShareable(new XRColocation::FColocationXR());
	ColocationXR->RegisterAsOpenXRExtension();
}

void FOculusXRColocationModule::ShutdownModule()
{
}

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE(FOculusXRColocationModule, OculusXRColocation)

#undef LOCTEXT_NAMESPACE
