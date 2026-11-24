// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRTilePropertiesHintExtensionPlugin.h"

#ifdef WITH_OCULUS_BRANCH

#include "IOpenXRHMDModule.h"
#include "OpenXRCore.h"
#include "DynamicRHI.h"
#include "HAL/IConsoleManager.h"
#include "OculusXRHMDPrivate.h"
#ifndef OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
#error "OculusXRHMDPrivate.h (above) must define OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN."
#endif

#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
#include "IVulkanDynamicRHI.h"
#endif

namespace OculusXR
{
	static TAutoConsoleVariable<bool> CVarTilePropertiesHintEnabled(
		TEXT("xr.MetaXRTilePropertiesHint.Enabled"),
		true,
		TEXT("Forward the GPU HW tile size to the runtime via XR_META_tile_properties_hint (tile-aligned foveation). Set to 0 to disable (retracts the active hint)."),
		ECVF_RenderThreadSafe);

	FString FTilePropertiesHintExtensionPlugin::GetDisplayName()
	{
		return TEXT("OculusXRTilePropertiesHint");
	}

	bool FTilePropertiesHintExtensionPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_META_TILE_PROPERTIES_HINT_EXTENSION_NAME);
		return true;
	}

	const void* FTilePropertiesHintExtensionPlugin::OnCreateInstance(IOpenXRHMDModule* InModule, const void* InNext)
	{
		bExtensionEnabled = InModule->IsExtensionEnabled(XR_META_TILE_PROPERTIES_HINT_EXTENSION_NAME);
		return InNext;
	}

	void FTilePropertiesHintExtensionPlugin::PostCreateInstance(XrInstance InInstance)
	{
		if (bExtensionEnabled)
		{
			XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrSetTilePropertiesHintMETA", (PFN_xrVoidFunction*)&xrSetTilePropertiesHintMETA));
			bExtensionEnabled = xrSetTilePropertiesHintMETA != nullptr;
		}
	}

	void FTilePropertiesHintExtensionPlugin::PostCreateSession(XrSession InSession)
	{
		// A new session has not received the hint yet; clear so OnBeginRendering re-submits for it.
		LastSubmittedTileSize = FIntPoint(0, 0);
		bWarnedSubmitFailure = false;
	}

	void FTilePropertiesHintExtensionPlugin::OnBeginRendering_RenderThread(XrSession InSession, FRDGBuilder& GraphBuilder)
	{
		if (!bExtensionEnabled || InSession == XR_NULL_HANDLE)
		{
			return;
		}

		// Runtime keeps the hint for the session, so disabling must actively retract it (zero-count; an all-zero tile is rejected).
		if (!CVarTilePropertiesHintEnabled.GetValueOnRenderThread())
		{
			if (LastSubmittedTileSize != FIntPoint(0, 0))
			{
				XrTilePropertiesHintMETA ClearHint{ XR_TYPE_TILE_PROPERTIES_HINT_META };
				ClearHint.next = nullptr;
				ClearHint.propertiesCount = 0;
				ClearHint.properties = nullptr;
				const XrResult ClearResult = xrSetTilePropertiesHintMETA(InSession, &ClearHint);
				if (XR_SUCCEEDED(ClearResult))
				{
					LastSubmittedTileSize = FIntPoint(0, 0);
					bWarnedSubmitFailure = false;
				}
				else if (!bWarnedSubmitFailure)
				{
					// Leave LastSubmittedTileSize non-zero so the clear retries next frame.
					bWarnedSubmitFailure = true;
					UE_LOG(LogHMD, Warning, TEXT("xrSetTilePropertiesHintMETA retract failed (%d)"), ClearResult);
				}
			}
			return;
		}

		// (0,0) until the first foveated framebuffer is created (one-frame startup latency).
		FIntPoint TileSize(0, 0);
#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
		if (GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan)
		{
			TileSize = GetIVulkanDynamicRHI()->RHIGetLastFoveationHwTilePixelSize();
		}
#endif

		if (TileSize.X <= 0 || TileSize.Y <= 0 || TileSize == LastSubmittedTileSize)
		{
			return;
		}

		XrTilePropertiesMETA TileProperties{ XR_TYPE_TILE_PROPERTIES_META };
		TileProperties.next = nullptr;
		TileProperties.tileDimensions.width = TileSize.X;
		TileProperties.tileDimensions.height = TileSize.Y;
		TileProperties.tileDimensions.depth = 1; // runtime rejects depth <= 0
		TileProperties.apronDimensions = { 0, 0 };
		TileProperties.origin = { 0, 0 };

		XrTilePropertiesHintMETA Hint{ XR_TYPE_TILE_PROPERTIES_HINT_META };
		Hint.next = nullptr;
		Hint.propertiesCount = 1;
		Hint.properties = &TileProperties;

		const XrResult Result = xrSetTilePropertiesHintMETA(InSession, &Hint);
		if (XR_SUCCEEDED(Result))
		{
			LastSubmittedTileSize = TileSize;
			bWarnedSubmitFailure = false;
			UE_LOG(LogHMD, Log, TEXT("Submitted XR_META_tile_properties_hint: %dx%d"), TileSize.X, TileSize.Y);
		}
		else if (!bWarnedSubmitFailure)
		{
			// Warn once, keep retrying silently.
			bWarnedSubmitFailure = true;
			UE_LOG(LogHMD, Warning, TEXT("xrSetTilePropertiesHintMETA failed (%d) for tile size %dx%d"), Result, TileSize.X, TileSize.Y);
		}
	}
} // namespace OculusXR

#endif // WITH_OCULUS_BRANCH
