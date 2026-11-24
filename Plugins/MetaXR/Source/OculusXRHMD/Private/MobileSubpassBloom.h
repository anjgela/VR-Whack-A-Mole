// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include "CoreMinimal.h"

// MobileSubpassPostProcess.h and the provider interface it declares exist only in
// the Meta engine fork, so the whole render path is gated. Keep CoreMinimal.h above
// this line -- it is what defines WITH_OCULUS_BRANCH.
#ifdef WITH_OCULUS_BRANCH

#include "MobileSubpassPostProcess.h"
#include "RenderGraphResources.h"
#include "RendererInterface.h"

class FFinalPostProcessSettings;
class FRDGBuilder;
class FSceneView;
class FRHITexture;
class UWorld;

namespace OculusXR
{
	void InitializeMobileSubpassBloomComponentRegistry_GameThread();
	void ShutdownMobileSubpassBloomComponentRegistry_GameThread();

	void SetMobileSubpassBloomComponentOwner_GameThread(
		const UWorld* World,
		uint64 OwnerId,
		bool bActive);

	bool IsMobileSubpassBloomComponentOwner_RenderThread(
		const FSceneView& View,
		uint64 OwnerId);

	struct FMobileSubpassBloomRenderSettings
	{
		float IntensityScale = 1.0f;
		FLinearColor TintScale = FLinearColor::White;
		bool bSunMerge = true;
		float SunSkyStartCm = 3000.0f;
		float SunSkyEndCm = 8000.0f;
		float SunIntensity = 4.0f;
		float SunHaloWeight = 0.5f;
		float SunCoreTightness = 350.0f;
		float SunHaloFalloff = 6.0f;
	};

	class FMobileSubpassBloom : public IMobileSubpassPostProcessProvider
	{
	public:
		FMobileSubpassBloom();
		virtual ~FMobileSubpassBloom();

		void SetRenderSettings_RenderThread(
			const FMobileSubpassBloomRenderSettings& InSettings);

		static int32 GetPyramidDownsampleFactor();

		void BuildPyramidAndReproject_RenderThread(
			FRDGBuilder& GraphBuilder,
			const FSceneView& View,
			FRHITexture* MotionVectorTexture,
			FRHITexture* DepthTexture);

		void ExtractBrightnessForNextFrame_RenderThread(
			FRDGBuilder& GraphBuilder,
			const FSceneView& View,
			FRDGTextureRef TonemapOutput,
			bool bSourceIsLDR);

		// Sun-merge runs at provider-registration, before this frame's base pass
		// writes depth, so it reads frame N's resolved depth on frame N+1: stash
		// from the post-tonemap callback, read via GetPrevFrameSceneDepthRHI next
		// frame. Needs r.Mobile.ForceDepthResolve=1 to keep the resolved depth.
		// Sampled at current-frame UVs (not warped), so the sky mask lags one
		// frame under head rotation -- acceptable for a 6-tap 1/8-res mask.
		void StashSceneDepthForNextFrame_RenderThread(
			FRDGBuilder& GraphBuilder,
			const FSceneView& View,
			FRDGTextureRef SceneDepthTexture);

		FRHITexture* GetPrevFrameSceneDepthRHI(const FSceneView& View) const;

		virtual void ProvideMobileSubpassPostProcessInputs(
			FRDGBuilder& GraphBuilder,
			const FSceneView& View,
			EMobileSubpassPostProcessSlot Slot,
			FMobileSubpassPostProcessInputs& Inout) override;

	private:
		bool IsExecutionEnabled() const;

		struct FPerViewState
		{
			TRefCountPtr<IPooledRenderTarget> PrevFrameBrightness;
			TRefCountPtr<IPooledRenderTarget> PrevFrameDepth;
			FRDGTextureRef CurrentReprojectedBloom = nullptr;
			FRDGTextureRef CurrentPyramidBlurred = nullptr;
			FRDGTextureRef CurrentDebugExtractRDG = nullptr;
			FMatrix44f PrevViewProjMatrix = FMatrix44f::Identity;
		};

		FPerViewState ViewStates[2];
		FMobileSubpassBloomRenderSettings RenderSettings;
		int32 GetViewStateIndex(const FSceneView& View) const;

		FMatrix44f FrozenPrevViewProj[2] = { FMatrix44f::Identity, FMatrix44f::Identity };
		uint32 FrozenPrevFrameNumber = MAX_uint32;

		FRDGTextureRef BuildDualFilterPyramid_RenderThread(
			FRDGBuilder& GraphBuilder,
			ERHIFeatureLevel::Type FeatureLevel,
			FRDGTextureRef Source,
			int32 NumLevels,
			const FFinalPostProcessSettings& PPS);

		FRDGTextureRef ReprojectBloom_RenderThread(
			FRDGBuilder& GraphBuilder,
			const FSceneView& View,
			FRDGTextureRef PrevBloomBlurred,
			FRHITexture* MotionVectorTexture,
			FRHITexture* DepthTexture,
			const FMatrix44f (&PrevViewProjMatrices)[2]);
	};

} // namespace OculusXR

#endif // WITH_OCULUS_BRANCH
