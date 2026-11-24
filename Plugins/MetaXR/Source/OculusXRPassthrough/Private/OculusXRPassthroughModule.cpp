// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRPassthroughModule.h"

DEFINE_LOG_CATEGORY(LogOculusXRPassthrough);

#define LOCTEXT_NAMESPACE "OculusXRPassthrough"

//-------------------------------------------------------------------------------------------------
// FOculusXRPassthroughModule
//-------------------------------------------------------------------------------------------------

FOculusXRPassthroughModule::FOculusXRPassthroughModule()
{
}

void FOculusXRPassthroughModule::StartupModule()
{
	PassthroughXR = MakeShareable(new XRPassthrough::FPassthroughXR());
	PassthroughXR->RegisterAsOpenXRExtension();
}

void FOculusXRPassthroughModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FOculusXRPassthroughModule, OculusXRPassthrough)

#undef LOCTEXT_NAMESPACE
