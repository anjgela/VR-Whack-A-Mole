// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once
#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "OpenXR/IOculusXRExtensionPlugin.h"
#include "SceneViewExtension.h"

#if !UE_VERSION_OLDER_THAN(5, 7, 0) && defined(WITH_OCULUS_BRANCH)

#include <khronos/openxr/meta_openxr_preview/metax1_temporal_pixel_synthesis.h>

class IOpenXRHMD;

DECLARE_LOG_CATEGORY_EXTERN(LogOculusTAAExtensionPlugin, Log, All);

namespace OculusXR
{
	class FTemporalAntiAliasingExtensionPlugin : public IOculusXRExtensionPlugin
	{
	public:
		FTemporalAntiAliasingExtensionPlugin();

		// IOpenXRExtensionPlugin
		virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		virtual const void* OnCreateInstance(IOpenXRHMDModule* InModule, const void* InNext) override;
		virtual void PostCreateSession(XrSession InSession) override;
		virtual void OnDestroySession(XrSession InSession) override;
		virtual const void* OnBeginProjectionView(XrSession InSession, int32 InLayerIndex, int32 InViewIndex, const void* InNext) override;
		virtual void* OnEnumerateViewConfigurationViews(XrInstance InInstance, XrSystemId InSystem, XrViewConfigurationType InViewConfigurationType, uint32_t InViewIndex, void* InNext) override;
		virtual bool GetRecommendedMotionVectorTextureSize(FIntPoint& OutSize) override;

		bool IsTAAEnabled() const;

	private:
		class FTAAViewExtension : public FHMDSceneViewExtension
		{
		public:
			FTAAViewExtension(const FAutoRegister& AutoRegister, FTemporalAntiAliasingExtensionPlugin* InPlugin);

			virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;

		private:
			FTemporalAntiAliasingExtensionPlugin* TAAPlugin = nullptr;
		};

		// Sizes the per-view TAA arrays: OpenXR stereo uses 2 views, quad-view foveated uses 4.
		static constexpr int32 kMaxViewCount = 4;

		bool bTAAExtensionAvailable = false;
		bool bIsMobileMultiViewEnabled = false;
		// Cached OpenXR HMD interface, used to source the motion vector layer descriptor so TAA can submit
		// its own pixel-synthesis (TPS) layer instead of riding the engine's frame synthesis layer.
		IOpenXRHMD* OpenXRHMD = nullptr;
		TSharedPtr<FTAAViewExtension> TAAViewExtension = nullptr;
		XrPixelSynthesisInfoMETAX1 TaaInfo[kMaxViewCount];
		// Chained into xrEnumerateViewConfigurationViews so the runtime fills in the TAA-recommended
		// motion vector rect (~eye size). Reported back to the engine via GetRecommendedMotionVectorTextureSize.
		XrViewPixelSynthesisConfigurationViewMETAX1 TaaConfigViews[kMaxViewCount];
	};

} // namespace OculusXR

#endif // !UE_VERSION_OLDER_THAN(5, 7, 0) && defined(WITH_OCULUS_BRANCH)
