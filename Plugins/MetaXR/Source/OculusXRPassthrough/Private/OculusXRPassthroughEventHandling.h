// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include "CoreMinimal.h"

namespace OculusXRPassthrough
{
	class FOculusXRPassthroughEventDelegates
	{
	public:
		DECLARE_MULTICAST_DELEGATE_OneParam(FOculusXRPassthroughLayerResumedDelegate, int /*layerId*/);
		static OCULUSXRPASSTHROUGH_API FOculusXRPassthroughLayerResumedDelegate OculusPassthroughLayerResumed;

	};

} // namespace OculusXRPassthrough
