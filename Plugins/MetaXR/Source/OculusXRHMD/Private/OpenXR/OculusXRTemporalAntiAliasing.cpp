// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRTemporalAntiAliasing.h"
#include "IOpenXRHMDModule.h"
#include "IOpenXRHMD.h"
#include "IXRTrackingSystem.h"
#include "OpenXRCore.h"
#include "StereoRenderUtils.h"
#include "SceneView.h"
#include "Engine/Engine.h"
#include <float.h>

#if !UE_VERSION_OLDER_THAN(5, 7, 0) && defined(WITH_OCULUS_BRANCH)

DEFINE_LOG_CATEGORY(LogOculusTAAExtensionPlugin);

namespace OculusXR
{
	FTemporalAntiAliasingExtensionPlugin::FTemporalAntiAliasingExtensionPlugin()
	{
		FMemory::Memzero(TaaInfo, sizeof(TaaInfo));
		FMemory::Memzero(TaaConfigViews, sizeof(TaaConfigViews));
	}

	bool FTemporalAntiAliasingExtensionPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_METAX1_TEMPORAL_PIXEL_SYNTHESIS_EXTENSION_NAME);
		return true;
	}

	const void* FTemporalAntiAliasingExtensionPlugin::OnCreateInstance(IOpenXRHMDModule* InModule, const void* InNext)
	{
		if (InModule)
		{
			const bool bFrameSynthesisAvailable =
				InModule->IsExtensionEnabled(XR_EXT_FRAME_SYNTHESIS_EXTENSION_NAME) || InModule->IsExtensionEnabled(XR_FB_SPACE_WARP_EXTENSION_NAME);
			bTAAExtensionAvailable = InModule->IsExtensionEnabled(XR_METAX1_TEMPORAL_PIXEL_SYNTHESIS_EXTENSION_NAME);

			UE_LOG(LogOculusTAAExtensionPlugin, Log, TEXT("[Mobile TAA] Extensions: FrameSynthesis=%d, PixelSynthesis=%d"),
				bFrameSynthesisAvailable, bTAAExtensionAvailable);
		}

		return InNext;
	}

	void FTemporalAntiAliasingExtensionPlugin::PostCreateSession(XrSession InSession)
	{
		UE::StereoRenderUtils::FStereoShaderAspects Aspects(GMaxRHIShaderPlatform);
		bIsMobileMultiViewEnabled = Aspects.IsMobileMultiViewEnabled();

		// Cache the OpenXR HMD interface so OnBeginProjectionView can source the motion vector layer
		// descriptor and submit our own pixel-synthesis (TPS) layer (independent of frame synthesis / space warp).
		if (GEngine && GEngine->XRSystem.IsValid())
		{
			OpenXRHMD = GEngine->XRSystem->GetIOpenXRHMD();
		}
		if (OpenXRHMD == nullptr)
		{
			UE_LOG(LogOculusTAAExtensionPlugin, Warning, TEXT("[Mobile TAA] Could not cache the OpenXR HMD interface; NativeOpenXR TAA will be inert (OnBeginProjectionView requires it)."));
		}

		TAAViewExtension = FSceneViewExtensions::NewExtension<FTAAViewExtension>(this);

		// One-time sanity check: the engine's MV pass is gated by r.Velocity.DirectlyRenderOpenXRMotionVectors
		// (read-only, cook-time); without it there are no velocities and TAA is a no-op.
		if (IsTAAEnabled())
		{
			static IConsoleVariable* CVarDirectlyRenderMotionVectors = IConsoleManager::Get().FindConsoleVariable(
				TEXT("r.Velocity.DirectlyRenderOpenXRMotionVectors"));
			if (CVarDirectlyRenderMotionVectors && CVarDirectlyRenderMotionVectors->GetInt() == 0)
			{
				UE_LOG(LogOculusTAAExtensionPlugin, Warning, TEXT("[Mobile TAA] TAA is enabled but r.Velocity.DirectlyRenderOpenXRMotionVectors=0; the engine will not render motion vectors, so TAA will have no effect. Set it to 1 in [ConsoleVariables] (read-only, applied at cook time)."));
			}
		}
	}

	void FTemporalAntiAliasingExtensionPlugin::OnDestroySession(XrSession InSession)
	{
		// Mirror FSpaceWarpExtensionPlugin: drop the scene-view extension (otherwise a second one is
		// registered on the next PostCreateSession) and clear the cached HMD pointer.
		TAAViewExtension.Reset();
		OpenXRHMD = nullptr;
	}

	void* FTemporalAntiAliasingExtensionPlugin::OnEnumerateViewConfigurationViews(
		XrInstance InInstance, XrSystemId InSystem, XrViewConfigurationType InViewConfigurationType, uint32_t InViewIndex, void* InNext)
	{
		if (!bTAAExtensionAvailable || InViewIndex >= (uint32_t)kMaxViewCount)
		{
			return InNext;
		}

		// Chain the TAA config view so xrEnumerateViewConfigurationViews fills the runtime's recommended
		// MV rect (~eye size), read back via GetRecommendedMotionVectorTextureSize() to size the MV swapchain.
		// Memzero each call so stale fields don't leak if the runtime re-enumerates without rewriting them.
		XrViewPixelSynthesisConfigurationViewMETAX1& ConfigView = TaaConfigViews[InViewIndex];
		FMemory::Memzero(&ConfigView, sizeof(ConfigView));
		ConfigView.type = XR_TYPE_VIEW_PIXEL_SYNTHESIS_CONFIGURATION_VIEW_METAX1;
		ConfigView.next = InNext;
		return &ConfigView;
	}

	bool FTemporalAntiAliasingExtensionPlugin::GetRecommendedMotionVectorTextureSize(FIntPoint& OutSize)
	{
		// Only contribute a size when TAA is requested (else let the FS/ASW recommendation drive the shared
		// MV swapchain). Gate on the CVar directly, not IsTAAEnabled(): the latter needs bIsMobileMultiViewEnabled,
		// which isn't set until PostCreateSession, after the MV swapchain is first allocated.
		static IConsoleVariable* CVarTAAEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.Oculus.TemporalAntiAliasing.Enable"));
		const bool bTAARequested = CVarTAAEnable && CVarTAAEnable->GetInt() != 0;
		if (!bTAAExtensionAvailable || !bTAARequested)
		{
			return false;
		}

		FIntPoint Size = FIntPoint::ZeroValue;
		for (int32 i = 0; i < kMaxViewCount; ++i)
		{
			Size.X = FMath::Max(Size.X, (int32)TaaConfigViews[i].recommendedMotionVectorImageRectWidth);
			Size.Y = FMath::Max(Size.Y, (int32)TaaConfigViews[i].recommendedMotionVectorImageRectHeight);
		}

		if (Size.X <= 0 || Size.Y <= 0)
		{
			return false;
		}

		// Scale the runtime-recommended motion vector size by the pixel-density CVar (1.0 = recommended).
		static IConsoleVariable* CVarMVPixelDensity = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.Oculus.MotionVectorPixelDensity"));
		const float PixelDensity = CVarMVPixelDensity ? CVarMVPixelDensity->GetFloat() : 1.0f;
		if (PixelDensity > 0.0f && PixelDensity != 1.0f)
		{
			Size.X = FMath::Max(1, FMath::RoundToInt(Size.X * PixelDensity));
			Size.Y = FMath::Max(1, FMath::RoundToInt(Size.Y * PixelDensity));
		}

		OutSize = Size;
		return true;
	}

	bool FTemporalAntiAliasingExtensionPlugin::IsTAAEnabled() const
	{
		static IConsoleVariable* CVarTAAEnable = IConsoleManager::Get().FindConsoleVariable(
			TEXT("r.Mobile.Oculus.TemporalAntiAliasing.Enable"));
		const bool bUserEnabled = CVarTAAEnable && CVarTAAEnable->GetInt() != 0;
		return bUserEnabled
			&& bTAAExtensionAvailable
			&& bIsMobileMultiViewEnabled;
	}

	const void* FTemporalAntiAliasingExtensionPlugin::OnBeginProjectionView(
		XrSession InSession, int32 InLayerIndex, int32 InViewIndex, const void* InNext)
	{
		check(IsInRenderingThread());

		if (!IsTAAEnabled() || InViewIndex < 0 || InViewIndex >= kMaxViewCount || OpenXRHMD == nullptr)
		{
			return InNext;
		}

		// Submit our OWN pixel-synthesis (TPS) layer from the engine's MV descriptor, independent of the
		// engine's frame-synthesis/space-warp layer -- so FS can stay disabled (it otherwise foveates the
		// MV swapchain with ASW geometry, corrupting the MV fragment density map).
		XrSwapchainSubImage MotionVectorSubImage = {};
		XrSwapchainSubImage MotionVectorDepthSubImage = {};
		XrPosef AppSpaceDeltaPose = {};
		float NearClip = 0.0f;
		XrFovf MotionVectorFov = {};
		if (!OpenXRHMD->GetMotionVectorLayerDesc_RenderThread(InViewIndex, MotionVectorSubImage, MotionVectorDepthSubImage, AppSpaceDeltaPose, NearClip, MotionVectorFov))
		{
			return InNext;
		}

		XrPixelSynthesisInfoMETAX1& Info = TaaInfo[InViewIndex];
		FMemory::Memzero(&Info, sizeof(Info));
		Info.type = XR_TYPE_PIXEL_SYNTHESIS_INFO_METAX1;
		Info.next = InNext;

		// Read the upscaling level once so the flag bit and quality value stay consistent within this submit.
		static IConsoleVariable* CVarTAAUpscaling = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.Oculus.TemporalAntiAliasing.Upscaling"));
		const int32 UpscalingLevel = CVarTAAUpscaling ? FMath::Clamp(CVarTAAUpscaling->GetInt(), 0, 4) : 0;

		Info.layerFlags = XR_PIXEL_SYNTHESIS_USE_DEPTH_BIT_METAX1;
		if (UpscalingLevel != 0)
		{
			Info.layerFlags |= XR_PIXEL_SYNTHESIS_ENABLE_UPSCALING_BIT_METAX1;
		}

		Info.motionVectorSubImage = MotionVectorSubImage;
		Info.motionVectorScale = { 1.0f, 1.0f };
		Info.motionVectorOffset = { 0.0f, 0.0f };
		Info.appSpaceDeltaPose = AppSpaceDeltaPose;
		Info.depthSubImage = MotionVectorDepthSubImage;
		Info.minDepth = 0.0f;
		Info.maxDepth = 1.0f;
		// Reverse-Z infinite far plane (UE convention): nearZ/farZ are deliberately inverted
		// (nearZ = +inf, farZ = the near clip distance) so the runtime reconstructs linear depth correctly.
		Info.nearZ = FLT_MAX;
		Info.farZ = NearClip;

		Info.motionVectorFov = MotionVectorFov;

		Info.stencilSubImage = {};
		Info.stencilBitMask = 0;
		Info.stencilValue = 0;

		// fovea/periphery quality (CVar 1-4 -> LOW..DYNAMIC) and fovea radius (CVar 1-4 -> SMALL..DYNAMIC)
		// map directly onto the pixel-synthesis enums; CVar 0 ("use default") keeps the runtime's High/Large quality.
		static_assert(
			XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_LOW_METAX1 == 1 && XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_MEDIUM_METAX1 == 2 && XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_HIGH_METAX1 == 3 && XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_DYNAMIC_METAX1 == 4 && XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_SMALL_METAX1 == 1 && XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_MEDIUM_METAX1 == 2 && XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_LARGE_METAX1 == 3 && XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_DYNAMIC_METAX1 == 4,
			"TPS quality/fovea-radius enums reordered; the CVar-level static_casts below assume 1-4 map to the documented levels.");
		static IConsoleVariable* CVarFoveaQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.Oculus.TemporalAntiAliasing.FoveaQualityLevel"));
		static IConsoleVariable* CVarPeripheryQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.Oculus.TemporalAntiAliasing.PeripheryQualityLevel"));
		static IConsoleVariable* CVarFoveaRadius = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.Oculus.TemporalAntiAliasing.FoveaRadius"));
		// Clamp to the valid 0-4 enum range so an out-of-range CVar value never casts to an invalid pixel-synthesis enum.
		const int32 FoveaQualityLevel = CVarFoveaQuality ? FMath::Clamp(CVarFoveaQuality->GetInt(), 0, 4) : 0;
		const int32 PeripheryQualityLevel = CVarPeripheryQuality ? FMath::Clamp(CVarPeripheryQuality->GetInt(), 0, 4) : 0;
		const int32 FoveaRadiusLevel = CVarFoveaRadius ? FMath::Clamp(CVarFoveaRadius->GetInt(), 0, 4) : 0;
		Info.foveaQuality = FoveaQualityLevel != 0
			? static_cast<XrPixelSynthesisQualityLevelMETAX1>(FoveaQualityLevel)
			: XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_HIGH_METAX1;
		Info.peripheryQuality = PeripheryQualityLevel != 0
			? static_cast<XrPixelSynthesisQualityLevelMETAX1>(PeripheryQualityLevel)
			: XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_HIGH_METAX1;
		Info.foveaRadius = FoveaRadiusLevel != 0
			? static_cast<XrPixelSynthesisFoveaRadiusMETAX1>(FoveaRadiusLevel)
			: XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_LARGE_METAX1;
		Info.upscalingQuality = static_cast<XrPixelSynthesisQualityLevelMETAX1>(UpscalingLevel);
		Info.resetAggregations = XR_FALSE;

		return &Info;
	}

	// -----------------------------------------------------------------------
	// FTAAViewExtension
	// -----------------------------------------------------------------------

	FTemporalAntiAliasingExtensionPlugin::FTAAViewExtension::FTAAViewExtension(
		const FAutoRegister& AutoRegister,
		FTemporalAntiAliasingExtensionPlugin* InPlugin)
		: FHMDSceneViewExtension(AutoRegister)
		, TAAPlugin(InPlugin)
	{
	}

	void FTemporalAntiAliasingExtensionPlugin::FTAAViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
	{
		check(IsInGameThread());

		if (TAAPlugin->IsTAAEnabled())
		{
			InViewFamily.EngineShowFlags.StereoMotionVectors = true;
		}
	}

} // namespace OculusXR

#endif // !UE_VERSION_OLDER_THAN(5, 7, 0) && defined(WITH_OCULUS_BRANCH)
