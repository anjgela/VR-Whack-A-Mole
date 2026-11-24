// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

// DO NOT REMOVE
// Force the newer bundled OpenXR header (from Thirdparty/KhronosOpenXR/) so XR_META_tile_properties_hint is
// defined; without it IOpenXRExtensionPlugin.h pulls in the engine's outdated header.
#include "khronos/openxr/openxr.h"

#include "IOpenXRExtensionPlugin.h"
#include "Math/IntPoint.h"

class FRDGBuilder;

namespace OculusXR
{
#ifdef WITH_OCULUS_BRANCH

	// Forwards the GPU hardware tile size (queried by VulkanRHI via VK_QCOM_tile_properties and exposed through
	// RHIGetLastFoveationHwTilePixelSize) to the runtime via xrSetTilePropertiesHintMETA, so the runtime aligns
	// its fragment-density map to whole hardware tiles (tile-aligned foveated rendering).
	class FTilePropertiesHintExtensionPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void RegisterOpenXRExtensionPlugin()
		{
			RegisterOpenXRExtensionModularFeature();
		}

		virtual FString GetDisplayName() override;
		virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		virtual const void* OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext) override;
		virtual void PostCreateInstance(XrInstance InInstance) override;
		virtual void PostCreateSession(XrSession InSession) override;
		virtual void OnBeginRendering_RenderThread(XrSession InSession, FRDGBuilder& GraphBuilder) override;

	private:
		bool bExtensionEnabled = false;
		bool bWarnedSubmitFailure = false;
		PFN_xrSetTilePropertiesHintMETA xrSetTilePropertiesHintMETA = nullptr;
		FIntPoint LastSubmittedTileSize = FIntPoint(0, 0);
	};

#endif // WITH_OCULUS_BRANCH
} // namespace OculusXR
