// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRColocationFunctions.h"
#include "OculusXRColocationFunctionsOpenXR.h"

TSharedPtr<IOculusXRColocationFunctions> IOculusXRColocationFunctions::ColocationFunctionsImpl = nullptr;
TSharedPtr<IOculusXRColocationFunctions> IOculusXRColocationFunctions::GetOculusXRColocationFunctionsImpl()
{
	if (ColocationFunctionsImpl == nullptr)
	{
		ColocationFunctionsImpl = MakeShared<FOculusXRColocationFunctionsOpenXR>();
	}

	return ColocationFunctionsImpl;
}
