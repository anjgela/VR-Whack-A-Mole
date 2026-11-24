// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "MobileSubpassBloom.h"

#ifdef WITH_OCULUS_BRANCH

#include "GenerateMips.h"
#include "GlobalShader.h"
#include "LightShaftRendering.h"
#include "PostProcess/PostProcessing.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "ScenePrivate.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "SystemTextures.h"
#include "TextureResource.h"
#include "Misc/EngineVersionComparison.h"

DEFINE_LOG_CATEGORY_STATIC(LogMobileSubpassBloom, Log, All);

// UE 5.8 renamed GetViewProjectionMatrix() to GetWorldToClip().
static FORCEINLINE FMatrix GetWorldToClipCompat(const FViewMatrices& ViewMatrices)
{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
	return ViewMatrices.GetViewProjectionMatrix();
#else
	return ViewMatrices.GetWorldToClip();
#endif
}

// Names the Adreno PIL / gpu.renderstages.oculus Perfetto rollup; inert on 5.8 (RDG_GPU_STAT_SCOPE deprecated).
DECLARE_GPU_STAT_NAMED(MobileSubpassBloom, TEXT("MobileSubpassBloom"));

namespace OculusXR
{
	// Renderer CVars may register after the plugin's first lookup; retry misses
	// instead of caching a nullptr.
	static IConsoleVariable* FindEngineCVarLazy(const TCHAR* Name, IConsoleVariable*& Cached)
	{
		if (Cached == nullptr)
		{
			Cached = IConsoleManager::Get().FindConsoleVariable(Name);
		}
		return Cached;
	}

	static constexpr float BloomSoftKneeWidth = 0.5f;

	// LDR fallback extracts from the composited tonemap output (already carries
	// last frame's bloom); floor the threshold so it can't feed back into itself.
	// 0.2 is empirical, from the feedback-loop investigation in D103424820.
	static constexpr float LDRBloomThresholdFloor = 0.2f;

	static constexpr float ReprojectEdgeFadePower = 0.85f;

	static IConsoleVariable* GetBloomQualityCVar()
	{
		static IConsoleVariable* CVar = nullptr;
		return FindEngineCVarLazy(TEXT("r.BloomQuality"), CVar);
	}

	static IConsoleVariable* GetSubpassBloomReprojectionModeCVar()
	{
		static IConsoleVariable* CVar = nullptr;
		return FindEngineCVarLazy(TEXT("r.Mobile.SubpassBloom.ReprojectionMode"), CVar);
	}

	static IConsoleVariable* GetSubpassBloomPyramidDepthCVar()
	{
		static IConsoleVariable* CVar = nullptr;
		return FindEngineCVarLazy(TEXT("r.Mobile.SubpassBloom.PyramidDepth"), CVar);
	}

	static IConsoleVariable* GetSubpassBloomExtractDownsampleCVar()
	{
		static IConsoleVariable* CVar = nullptr;
		return FindEngineCVarLazy(TEXT("r.Mobile.SubpassBloom.ExtractDownsample"), CVar);
	}

	// Brightness and depth extraction must derive the same total downsample.
	struct FExtractDownsampleTarget
	{
		int32 TotalDownsample;
		FIntPoint DstSize;
	};

	static FExtractDownsampleTarget ComputeExtractDownsampleTarget(int32 BaseDownsample, FIntPoint SrcSize)
	{
		IConsoleVariable* ExtractDownCVar = GetSubpassBloomExtractDownsampleCVar();
		const int32 ExtraDown = ExtractDownCVar != nullptr ? FMath::Clamp(ExtractDownCVar->GetInt(), 1, 4) : 1;
		const int32 TotalDownsample = BaseDownsample * ExtraDown;
		return FExtractDownsampleTarget{
			TotalDownsample,
			FIntPoint(FMath::Max(1, SrcSize.X / TotalDownsample), FMath::Max(1, SrcSize.Y / TotalDownsample))
		};
	}

	static IConsoleVariable* GetSubpassBloomFastDownsampleCVar()
	{
		static IConsoleVariable* CVar = nullptr;
		return FindEngineCVarLazy(TEXT("r.Mobile.SubpassBloom.FastDownsample"), CVar);
	}

	static TAutoConsoleVariable<int32> CVarSubpassBloomSunDebugViz(
		TEXT("r.Mobile.SubpassBloom.SunDebugViz"), 0,
		TEXT("Visualize the sun-shaft depth occlusion. 0=off, 1=SkyMask (clear\n")
			TEXT("path to sun = white, blocked by geometry = dark), 2=SkyMask x\n")
				TEXT("sun profile (the shaft contribution)."),
		ECVF_RenderThreadSafe);
	static TAutoConsoleVariable<float> CVarSubpassBloomSunCenterX(
		TEXT("r.Mobile.SubpassBloom.SunCenterX"), -1.0f,
		TEXT("Override sun-shaft screen X in UV (0..1). <0 = use light shaft."),
		ECVF_RenderThreadSafe);
	static TAutoConsoleVariable<float> CVarSubpassBloomSunCenterY(
		TEXT("r.Mobile.SubpassBloom.SunCenterY"), 0.3f,
		TEXT("Override sun-shaft screen Y in UV (0..1). Paired with SunCenterX."),
		ECVF_RenderThreadSafe);

	static TAutoConsoleVariable<int32> CVarSubpassBloomCompositeMipChain(
		TEXT("r.Mobile.SubpassBloom.CompositeMipChain"), 1,
		TEXT("Generate a mip chain on the reproject output for a soft composite tap. "
			 "1=on (default), 0=off (single mip, sharp)."),
		ECVF_RenderThreadSafe);

	static TAutoConsoleVariable<int32> CVarSubpassBloomBaseDownsample(
		TEXT("r.Mobile.SubpassBloom.BaseDownsample"), 6,
		TEXT("Base pyramid downsample factor (eye-buffer / N). 6 = 1/6-res (default), 4 = 1/4-res, 8 = 1/8-res."),
		ECVF_RenderThreadSafe);

	int32 FMobileSubpassBloom::GetPyramidDownsampleFactor()
	{
		IConsoleVariable* CVar = GetBloomQualityCVar();
		const int32 Quality = CVar != nullptr ? CVar->GetInt() : 1;
		if (Quality <= 0)
		{
			return 0;
		}
		return FMath::Clamp(CVarSubpassBloomBaseDownsample.GetValueOnRenderThread(), 1, 16);
	}

	class FMobileSubpassBloomExtractCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FMobileSubpassBloomExtractCS);
		SHADER_USE_PARAMETER_STRUCT(FMobileSubpassBloomExtractCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, SrcTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SrcSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2DArray<float4>, DstTexture)
		SHADER_PARAMETER(FVector2f, SrcSizeInverse)
		SHADER_PARAMETER(FVector2f, DstSize)
		SHADER_PARAMETER(float, Threshold)
		SHADER_PARAMETER(float, Softness)
		SHADER_PARAMETER(uint32, bFastDownsample)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsMobilePlatform(Parameters.Platform);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
		}
	};
	IMPLEMENT_GLOBAL_SHADER(FMobileSubpassBloomExtractCS, "/Plugin/OculusXR/Private/MobileSubpassBloom.usf", "BrightnessExtractCS", SF_Compute);

	class FDualFilterDownCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FDualFilterDownCS);
		SHADER_USE_PARAMETER_STRUCT(FDualFilterDownCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, SrcTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SrcSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2DArray<float4>, DstTexture)
		SHADER_PARAMETER(FVector2f, SrcSizeInverse)
		SHADER_PARAMETER(FVector2f, DstSize)
		SHADER_PARAMETER(uint32, bFastDownsample)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsMobilePlatform(Parameters.Platform);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
		}
	};
	IMPLEMENT_GLOBAL_SHADER(FDualFilterDownCS, "/Plugin/OculusXR/Private/MobileSubpassBloom.usf", "DualFilterDownCS", SF_Compute);

	class FDualFilterUpCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FDualFilterUpCS);
		SHADER_USE_PARAMETER_STRUCT(FDualFilterUpCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, SrcLowTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, SrcHighTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SrcSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2DArray<float4>, DstTexture)
		SHADER_PARAMETER(FVector2f, SrcLowSizeInverse)
		SHADER_PARAMETER(FVector2f, DstSize)
		SHADER_PARAMETER(FVector3f, HighTint)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsMobilePlatform(Parameters.Platform);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
		}
	};
	IMPLEMENT_GLOBAL_SHADER(FDualFilterUpCS, "/Plugin/OculusXR/Private/MobileSubpassBloom.usf", "DualFilterUpCS", SF_Compute);

	class FReprojectBloomCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FReprojectBloomCS);
		SHADER_USE_PARAMETER_STRUCT(FReprojectBloomCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, PrevBloomTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, PrevFrameDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, MotionVectorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, BilinearClampSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, DepthPointSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2DArray<float4>, DstTexture)
		SHADER_PARAMETER(FVector2f, DstSize)
		SHADER_PARAMETER(FVector2f, DstSizeInverse)
		SHADER_PARAMETER(int32, ReprojectionMode)
		SHADER_PARAMETER(uint32, bHasMotionVectors)
		SHADER_PARAMETER(uint32, bHasDepth)
		SHADER_PARAMETER(FVector4f, InvDeviceZToWorldZTransform)
		SHADER_PARAMETER(FVector2f, SunSkyScaleBias)
		SHADER_PARAMETER(float, EdgeFadePower)
		SHADER_PARAMETER_ARRAY(FMatrix44f, ClipToPrevClip, [2])
		SHADER_PARAMETER(int32, bSunMergeEnabled)
		SHADER_PARAMETER(FVector4f, SunCenterUVLeftRight)
		SHADER_PARAMETER(FVector4f, SunColor)
		SHADER_PARAMETER(FVector4f, SunProfileParams)
		SHADER_PARAMETER(int32, SunDebugMode)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsMobilePlatform(Parameters.Platform);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
		}
	};
	IMPLEMENT_GLOBAL_SHADER(FReprojectBloomCS, "/Plugin/OculusXR/Private/MobileSubpassBloom.usf", "ReprojectBloomCS", SF_Compute);

	class FDepthDownsampleCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FDepthDownsampleCS);
		SHADER_USE_PARAMETER_STRUCT(FDepthDownsampleCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, SrcDepthTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, DepthPointSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2DArray<float>, DstDepthTexture)
		SHADER_PARAMETER(FVector2f, DepthDstSize)
		SHADER_PARAMETER(FVector2f, DepthSrcSizeInverse)
		SHADER_PARAMETER(int32, DepthDownsampleFactor)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsMobilePlatform(Parameters.Platform);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
		}
	};
	IMPLEMENT_GLOBAL_SHADER(FDepthDownsampleCS, "/Plugin/OculusXR/Private/MobileSubpassBloom.usf", "DepthDownsampleCS", SF_Compute);

	FMobileSubpassBloom::FMobileSubpassBloom() = default;
	FMobileSubpassBloom::~FMobileSubpassBloom() = default;

	bool FMobileSubpassBloom::IsExecutionEnabled() const
	{
		return IsMobileSubpassBloomEnabledAtInit();
	}

	void FMobileSubpassBloom::SetRenderSettings_RenderThread(
		const FMobileSubpassBloomRenderSettings& InSettings)
	{
		check(IsInRenderingThread());
		RenderSettings = InSettings;
	}

	int32 FMobileSubpassBloom::GetViewStateIndex(const FSceneView& View) const
	{
		// Instanced stereo would put both eyes in one view sharing slot 0; that
		// config is not exercised on this path.
		const int32 Idx = View.StereoViewIndex;
		return FMath::Clamp(Idx, 0, 1);
	}

	void FMobileSubpassBloom::ExtractBrightnessForNextFrame_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGTextureRef TonemapOutput,
		bool bSourceIsLDR)
	{
		if (!IsExecutionEnabled() || TonemapOutput == nullptr)
		{
			return;
		}
		const int32 Downsample = GetPyramidDownsampleFactor();
		if (Downsample == 0)
		{
			return;
		}

		RDG_GPU_STAT_SCOPE(GraphBuilder, MobileSubpassBloom);
		RDG_EVENT_SCOPE(GraphBuilder, "MobileSubpassBloom.PostBasepassExtract");

		const int32 NumViews = FMath::Max<int32>(1, TonemapOutput->Desc.ArraySize);
		const FIntPoint SrcSize = TonemapOutput->Desc.Extent;
		const FIntPoint DstSize = ComputeExtractDownsampleTarget(Downsample, SrcSize).DstSize;

		FRDGTextureDesc Desc = FRDGTextureDesc::Create2DArray(
			DstSize,
			PF_FloatR11G11B10,
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV,
			NumViews);
		FRDGTextureRef BrightnessRT = GraphBuilder.CreateTexture(Desc, TEXT("MobileSubpassBloom.BrightnessExtract"));

		FMobileSubpassBloomExtractCS::FParameters* Params = GraphBuilder.AllocParameters<FMobileSubpassBloomExtractCS::FParameters>();
		Params->SrcTexture = TonemapOutput;
		Params->SrcSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		Params->DstTexture = GraphBuilder.CreateUAV(BrightnessRT);
		Params->SrcSizeInverse = FVector2f(1.0f / SrcSize.X, 1.0f / SrcSize.Y);
		Params->DstSize = FVector2f(DstSize.X, DstSize.Y);
		// UE uses -1 for pass-through; only the LDR fallback applies the feedback floor.
		const float UserBloomThreshold = View.FinalPostProcessSettings.BloomThreshold;
		const float ResolvedThreshold = FMath::Max(0.0f, UserBloomThreshold);
		Params->Threshold = bSourceIsLDR
			? FMath::Max(LDRBloomThresholdFloor, ResolvedThreshold)
			: ResolvedThreshold;
		Params->Softness = BloomSoftKneeWidth;
		IConsoleVariable* FastDownCVar = GetSubpassBloomFastDownsampleCVar();
		const bool bFastDown = FastDownCVar != nullptr && FastDownCVar->GetInt() != 0;
		Params->bFastDownsample = bFastDown ? 1u : 0u;

		TShaderMapRef<FMobileSubpassBloomExtractCS> ComputeShader(GetGlobalShaderMap(View.GetFeatureLevel()));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("MobileSubpassBloom.Extract"),
			ComputeShader,
			Params,
			FComputeShaderUtils::GetGroupCount(FIntVector(DstSize.X, DstSize.Y, NumViews), FIntVector(8, 8, 1)));

		const int32 ViewIdx = GetViewStateIndex(View);
		GraphBuilder.QueueTextureExtraction(BrightnessRT, &ViewStates[ViewIdx].PrevFrameBrightness);
	}

	void FMobileSubpassBloom::StashSceneDepthForNextFrame_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGTextureRef SceneDepthTexture)
	{
		if (SceneDepthTexture == nullptr || !RenderSettings.bSunMerge)
		{
			// A transient null depth keeps the last valid history; disabling sun merge releases it.
			if (!RenderSettings.bSunMerge)
			{
				ViewStates[GetViewStateIndex(View)].PrevFrameDepth.SafeRelease();
			}
			return;
		}
		const int32 Downsample = GetPyramidDownsampleFactor();
		if (Downsample == 0)
		{
			return;
		}

		RDG_GPU_STAT_SCOPE(GraphBuilder, MobileSubpassBloom);
		RDG_EVENT_SCOPE(GraphBuilder, "MobileSubpassBloom.DepthDownsample");

		// Mobile MSAA depth is memoryless after the tile; downsample it in-pass.
		const int32 NumViews = FMath::Max<int32>(1, SceneDepthTexture->Desc.ArraySize);
		const FIntPoint SrcSize = SceneDepthTexture->Desc.Extent;
		const FExtractDownsampleTarget Target = ComputeExtractDownsampleTarget(Downsample, SrcSize);
		const int32 TotalDownsample = Target.TotalDownsample;
		const FIntPoint DstSize = Target.DstSize;

		FRDGTextureDesc Desc = FRDGTextureDesc::Create2DArray(
			DstSize,
			PF_R32_FLOAT,
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV,
			NumViews);
		FRDGTextureRef DepthRT = GraphBuilder.CreateTexture(Desc, TEXT("MobileSubpassBloom.DepthDownsample"));

		FDepthDownsampleCS::FParameters* Params = GraphBuilder.AllocParameters<FDepthDownsampleCS::FParameters>();
		Params->SrcDepthTexture = SceneDepthTexture;
		Params->DepthPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		Params->DstDepthTexture = GraphBuilder.CreateUAV(DepthRT);
		Params->DepthDstSize = FVector2f(DstSize.X, DstSize.Y);
		Params->DepthSrcSizeInverse = FVector2f(1.0f / SrcSize.X, 1.0f / SrcSize.Y);
		Params->DepthDownsampleFactor = TotalDownsample;

		TShaderMapRef<FDepthDownsampleCS> ComputeShader(GetGlobalShaderMap(View.GetFeatureLevel()));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("MobileSubpassBloom.DepthDownsample"),
			ComputeShader,
			Params,
			FComputeShaderUtils::GetGroupCount(FIntVector(DstSize.X, DstSize.Y, NumViews), FIntVector(8, 8, 1)));

		const int32 ViewIdx = GetViewStateIndex(View);
		GraphBuilder.QueueTextureExtraction(DepthRT, &ViewStates[ViewIdx].PrevFrameDepth);
	}

	FRHITexture* FMobileSubpassBloom::GetPrevFrameSceneDepthRHI(const FSceneView& View) const
	{
		if (!RenderSettings.bSunMerge)
		{
			return nullptr;
		}
		const TRefCountPtr<IPooledRenderTarget>& Depth = ViewStates[GetViewStateIndex(View)].PrevFrameDepth;
		return Depth.IsValid() ? Depth->GetRHI() : nullptr;
	}

	FRDGTextureRef FMobileSubpassBloom::BuildDualFilterPyramid_RenderThread(
		FRDGBuilder& GraphBuilder,
		ERHIFeatureLevel::Type FeatureLevel,
		FRDGTextureRef Source,
		int32 NumLevels,
		const FFinalPostProcessSettings& PPS)
	{
		// Up-loop level L merges mip L+1, matching stock Bloom(N)Tint semantics.
		auto TintForLevel = [&PPS](int32 Level) -> FVector3f {
			const FLinearColor& C = (Level <= 0) ? PPS.Bloom1Tint
				: (Level == 1)					 ? PPS.Bloom2Tint
				: (Level == 2)					 ? PPS.Bloom3Tint
				: (Level == 3)					 ? PPS.Bloom4Tint
				: (Level == 4)					 ? PPS.Bloom5Tint
												 : PPS.Bloom6Tint;
			return FVector3f(C.R, C.G, C.B);
		};
		IConsoleVariable* FastDownCVar = GetSubpassBloomFastDownsampleCVar();
		const bool bFastDown = FastDownCVar != nullptr && FastDownCVar->GetInt() != 0;

		const FIntPoint SrcSize = Source->Desc.Extent;
		const int32 NumViews = FMath::Max<int32>(1, Source->Desc.ArraySize);
		TArray<FRDGTextureRef, TInlineAllocator<7>> DownLevels;
		DownLevels.Add(Source);

		FRDGTextureRef Cur = Source;
		FIntPoint CurSize = SrcSize;
		for (int32 Level = 0; Level < NumLevels; ++Level)
		{
			const FIntPoint NextSize(FMath::Max(1, CurSize.X / 2), FMath::Max(1, CurSize.Y / 2));
			FRDGTextureDesc Desc = FRDGTextureDesc::Create2DArray(
				NextSize,
				PF_FloatR11G11B10,
				FClearValueBinding::Black,
				TexCreate_ShaderResource | TexCreate_UAV,
				NumViews);
			FRDGTextureRef Down = GraphBuilder.CreateTexture(Desc, TEXT("MobileSubpassBloom.Down"));

			FDualFilterDownCS::FParameters* Params = GraphBuilder.AllocParameters<FDualFilterDownCS::FParameters>();
			Params->SrcTexture = Cur;
			Params->SrcSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			Params->DstTexture = GraphBuilder.CreateUAV(Down);
			Params->SrcSizeInverse = FVector2f(1.0f / CurSize.X, 1.0f / CurSize.Y);
			Params->DstSize = FVector2f(NextSize.X, NextSize.Y);
			Params->bFastDownsample = bFastDown ? 1u : 0u;

			TShaderMapRef<FDualFilterDownCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("MobileSubpassBloom.DownLevel%d", Level),
				ComputeShader,
				Params,
				FComputeShaderUtils::GetGroupCount(FIntVector(NextSize.X, NextSize.Y, NumViews), FIntVector(8, 8, 1)));

			DownLevels.Add(Down);
			Cur = Down;
			CurSize = NextSize;
		}

		for (int32 Level = NumLevels - 1; Level >= 0; --Level)
		{
			FRDGTextureRef Lower = Cur;
			FRDGTextureRef Higher = DownLevels[Level];
			const FIntPoint UpSize = Higher->Desc.Extent;
			FRDGTextureDesc Desc = FRDGTextureDesc::Create2DArray(
				UpSize,
				PF_FloatR11G11B10,
				FClearValueBinding::Black,
				TexCreate_ShaderResource | TexCreate_UAV,
				NumViews);
			FRDGTextureRef Up = GraphBuilder.CreateTexture(Desc, TEXT("MobileSubpassBloom.Up"));

			FDualFilterUpCS::FParameters* Params = GraphBuilder.AllocParameters<FDualFilterUpCS::FParameters>();
			Params->SrcLowTexture = Lower;
			Params->SrcHighTexture = Higher;
			Params->SrcSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			Params->DstTexture = GraphBuilder.CreateUAV(Up);
			Params->SrcLowSizeInverse = FVector2f(1.0f / Lower->Desc.Extent.X, 1.0f / Lower->Desc.Extent.Y);
			Params->DstSize = FVector2f(UpSize.X, UpSize.Y);
			Params->HighTint = TintForLevel(Level);

			TShaderMapRef<FDualFilterUpCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("MobileSubpassBloom.UpLevel%d", Level),
				ComputeShader,
				Params,
				FComputeShaderUtils::GetGroupCount(FIntVector(UpSize.X, UpSize.Y, NumViews), FIntVector(8, 8, 1)));

			Cur = Up;
		}
		return Cur;
	}

	FRDGTextureRef FMobileSubpassBloom::ReprojectBloom_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRDGTextureRef PrevBloomBlurred,
		FRHITexture* MotionVectorTexture,
		FRHITexture* DepthTexture,
		const FMatrix44f (&PrevViewProjMatrices)[2])
	{
		const int32 NumViews = FMath::Max<int32>(1, PrevBloomBlurred->Desc.ArraySize);
		const FIntPoint Size = PrevBloomBlurred->Desc.Extent;
		// The composite samples mip 2; provide three mips instead of clamping to the sharp base.
		const bool bMipChain = CVarSubpassBloomCompositeMipChain.GetValueOnRenderThread() != 0;
		const int32 MaxMips = FMath::FloorLog2(static_cast<uint32>(FMath::Max(Size.X, Size.Y))) + 1;
		const uint8 NumMips = bMipChain ? static_cast<uint8>(FMath::Clamp(3, 1, MaxMips)) : 1;
		// HDR power map: bloom values can exceed 1.0 so we need a float format.
		FRDGTextureDesc Desc = FRDGTextureDesc::Create2DArray(
			Size,
			PF_FloatR11G11B10,
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable,
			NumViews,
			NumMips);
		FRDGTextureRef Reprojected = GraphBuilder.CreateTexture(Desc, TEXT("MobileSubpassBloom.Reprojected"));

		// External motion/depth textures may be null; bind the array black dummy.
		// Without motion vectors, modes 1/2 use ClipToPrevClip; mode 0 disables reprojection.
		// Depth only gates sun occlusion. Z=0.5 handles rotation; translation is approximate.
		FRDGTextureRef MVTex = nullptr;
		FRDGTextureRef DepthTex = nullptr;
		if (MotionVectorTexture != nullptr)
		{
			MVTex = RegisterExternalTexture(GraphBuilder, MotionVectorTexture, TEXT("MobileSubpassBloom.MotionVectors"));
		}
		else
		{
			MVTex = GSystemTextures.GetBlackArrayDummy(GraphBuilder);
		}
		if (DepthTexture != nullptr)
		{
			DepthTex = RegisterExternalTexture(GraphBuilder, DepthTexture, TEXT("MobileSubpassBloom.Depth"));
		}
		else
		{
			DepthTex = GSystemTextures.GetBlackArrayDummy(GraphBuilder);
		}

		FReprojectBloomCS::FParameters* Params = GraphBuilder.AllocParameters<FReprojectBloomCS::FParameters>();
		Params->PrevBloomTexture = PrevBloomBlurred;
		Params->PrevFrameDepthTexture = DepthTex;
		Params->MotionVectorTexture = MVTex;
		Params->BilinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		Params->DepthPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		Params->DstTexture = GraphBuilder.CreateUAV(Reprojected);
		Params->DstSize = FVector2f(Size.X, Size.Y);
		Params->DstSizeInverse = FVector2f(1.0f / Size.X, 1.0f / Size.Y);

		IConsoleVariable* ModeCVar = GetSubpassBloomReprojectionModeCVar();
		Params->ReprojectionMode = ModeCVar != nullptr ? FMath::Clamp(ModeCVar->GetInt(), 0, 2) : 2;
		Params->bHasMotionVectors = (MotionVectorTexture != nullptr) ? 1u : 0u;
		Params->bHasDepth = (DepthTexture != nullptr) ? 1u : 0u;
		Params->InvDeviceZToWorldZTransform = FVector4f(View.InvDeviceZToWorldZTransform);
		{
			const float SkyStart = RenderSettings.SunSkyStartCm;
			// Keep the reciprocal finite; invalid bands are warned at the game-thread boundary.
			const float SkyEnd = FMath::Max(RenderSettings.SunSkyEndCm, SkyStart + 1.0f);
			const float SkyScale = 1.0f / (SkyEnd - SkyStart);
			Params->SunSkyScaleBias = FVector2f(SkyScale, -SkyStart * SkyScale);
		}
		Params->EdgeFadePower = ReprojectEdgeFadePower;

		// Each slice needs its matching eye matrices; sharing one eye shifts the other by the IPD.
		Params->ClipToPrevClip[0] = FMatrix44f::Identity;
		Params->ClipToPrevClip[1] = FMatrix44f::Identity;
		const int32 NumFamilyViews = View.Family != nullptr ? View.Family->Views.Num() : 0;
		const int32 NumEyesToFill = FMath::Min(NumViews, 2);
		for (int32 EyeIdx = 0; EyeIdx < NumEyesToFill; ++EyeIdx)
		{
			const FSceneView* EyeView = (EyeIdx < NumFamilyViews) ? View.Family->Views[EyeIdx] : &View;
			if (EyeView == nullptr)
			{
				continue;
			}
			const FMatrix44f EyeCurViewProj = FMatrix44f(GetWorldToClipCompat(EyeView->ViewMatrices));
			const FMatrix44f EyeInvCurViewProj = EyeCurViewProj.Inverse();
			Params->ClipToPrevClip[EyeIdx] = EyeInvCurViewProj * PrevViewProjMatrices[EyeIdx];
		}

		// MobileLightShaft is FViewInfo-private; render-thread family views are FViewInfo.
		Params->bSunMergeEnabled = 0;
		Params->SunCenterUVLeftRight = FVector4f(0.f, 0.f, 0.f, 0.f);
		Params->SunColor = FVector4f(0.f, 0.f, 0.f, 0.f);
		Params->SunProfileParams = FVector4f(
			RenderSettings.SunIntensity,
			// Keep exponential shape parameters positive to avoid Inf/NaN.
			FMath::Max(UE_KINDA_SMALL_NUMBER, RenderSettings.SunCoreTightness),
			FMath::Max(UE_KINDA_SMALL_NUMBER, RenderSettings.SunHaloFalloff),
			RenderSettings.SunHaloWeight);
		Params->SunDebugMode = CVarSubpassBloomSunDebugViz.GetValueOnRenderThread();
		{
			FVector2f SunUV[2] = { FVector2f::ZeroVector, FVector2f::ZeroVector };
			bool bEyeSet[2] = { false, false };
			FLinearColor SunColor = FLinearColor::Transparent;
			bool bAnyEye = false;
			int32 FirstSetEye = 0;
			for (int32 EyeIdx = 0; EyeIdx < NumEyesToFill; ++EyeIdx)
			{
				const FSceneView* SV = (EyeIdx < NumFamilyViews) ? View.Family->Views[EyeIdx] : &View;
				if (SV == nullptr)
				{
					continue;
				}
				const FViewInfo* VI = static_cast<const FViewInfo*>(SV);
				if (VI->MobileLightShaft.IsSet())
				{
					const FVector2D NDC = VI->MobileLightShaft->Center;
					SunUV[EyeIdx] = FVector2f(static_cast<float>((NDC.X + 1.0) * 0.5), static_cast<float>((1.0 - NDC.Y) * 0.5));
					bEyeSet[EyeIdx] = true;
					if (!bAnyEye)
					{
						SunColor = VI->MobileLightShaft->ColorMask;
						FirstSetEye = EyeIdx;
					}
					bAnyEye = true;
				}
			}
			if (bAnyEye)
			{
				// Mirror a missing eye so it cannot paint a shaft from UV (0,0).
				for (int32 EyeIdx = 0; EyeIdx < NumEyesToFill; ++EyeIdx)
				{
					if (!bEyeSet[EyeIdx])
					{
						SunUV[EyeIdx] = SunUV[FirstSetEye];
					}
				}
				Params->bSunMergeEnabled = 1;
				Params->SunCenterUVLeftRight = FVector4f(SunUV[0].X, SunUV[0].Y, SunUV[1].X, SunUV[1].Y);
				// Normalize to hue because ColorMask can be nearly black.
				const float MaxC = FMath::Max3(SunColor.R, SunColor.G, SunColor.B);
				const FLinearColor SunHue = (MaxC > 1e-4f) ? (SunColor * (1.0f / MaxC)) : FLinearColor(1.0f, 0.85f, 0.6f);
				Params->SunColor = FVector4f(SunHue.R, SunHue.G, SunHue.B, 0.0f);
			}

			const float OverrideX = CVarSubpassBloomSunCenterX.GetValueOnRenderThread();
			if (OverrideX >= 0.0f)
			{
				const float OverrideY = CVarSubpassBloomSunCenterY.GetValueOnRenderThread();
				Params->bSunMergeEnabled = 1;
				Params->SunCenterUVLeftRight = FVector4f(OverrideX, OverrideY, OverrideX, OverrideY);
				if (Params->SunColor.X + Params->SunColor.Y + Params->SunColor.Z < 1e-4f)
				{
					Params->SunColor = FVector4f(1.0f, 0.85f, 0.6f, 0.0f);
				}
			}

			if (!RenderSettings.bSunMerge)
			{
				Params->bSunMergeEnabled = 0;
			}
		}

		TShaderMapRef<FReprojectBloomCS> ComputeShader(GetGlobalShaderMap(View.GetFeatureLevel()));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("MobileSubpassBloom.Reproject"),
			ComputeShader,
			Params,
			FComputeShaderUtils::GetGroupCount(FIntVector(Size.X, Size.Y, NumViews), FIntVector(8, 8, 1)));

		// The pyramid already requires R11G11B10 UAV writes; RenderTargetable enables
		// FGenerateMips' raster fallback when compute mip generation is unavailable.
		if (Reprojected->Desc.NumMips > 1)
		{
			FGenerateMips::Execute(GraphBuilder, View.GetFeatureLevel(), Reprojected, FGenerateMipsParams());
		}

		return Reprojected;
	}

	void FMobileSubpassBloom::BuildPyramidAndReproject_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		FRHITexture* MotionVectorTexture,
		FRHITexture* DepthTexture)
	{
		if (!IsExecutionEnabled())
		{
			return;
		}
		const int32 ViewIdx = GetViewStateIndex(View);
		FPerViewState& State = ViewStates[ViewIdx];

		// Clear history when bloom is disabled, or stale bloom would keep reprojecting.
		if (GetPyramidDownsampleFactor() <= 0)
		{
			State.PrevFrameBrightness.SafeRelease();
			State.PrevFrameDepth.SafeRelease();
			State.CurrentReprojectedBloom = nullptr;
			State.CurrentPyramidBlurred = nullptr;
			State.CurrentDebugExtractRDG = nullptr;
			return;
		}

		// Snapshot N-1 matrices on the frame's first invocation; the block below then
		// advances ViewStates to the current frame. On the shipping single-pass-stereo
		// path this fires once per frame (the secondary view is culled), so the snapshot
		// is defensive: if a future multi-pass path ever invoked this per eye, a later
		// call would otherwise reproject against the current matrix instead of N-1.
		const uint32 FrameNum = (View.Family != nullptr) ? View.Family->FrameNumber : GFrameNumberRenderThread;
		const bool bFirstInvocationThisFrame = (FrameNum != FrozenPrevFrameNumber);

		if (bFirstInvocationThisFrame)
		{
			FrozenPrevViewProj[0] = ViewStates[0].PrevViewProjMatrix;
			FrozenPrevViewProj[1] = ViewStates[1].PrevViewProjMatrix;
			FrozenPrevFrameNumber = FrameNum;

			if (View.Family != nullptr)
			{
				const int32 NumFamilyViews = View.Family->Views.Num();
				for (int32 i = 0; i < FMath::Min(NumFamilyViews, 2); ++i)
				{
					if (View.Family->Views[i] != nullptr)
					{
						ViewStates[i].PrevViewProjMatrix = FMatrix44f(
							GetWorldToClipCompat(View.Family->Views[i]->ViewMatrices));
					}
				}
			}
			else
			{
				// Without a family, mirror the current matrix so neither slot stays stale.
				const FMatrix44f Cur = FMatrix44f(GetWorldToClipCompat(View.ViewMatrices));
				ViewStates[0].PrevViewProjMatrix = Cur;
				ViewStates[1].PrevViewProjMatrix = Cur;
			}
		}

		const FMatrix44f(&PrevViewProjMatrices)[2] = FrozenPrevViewProj;

		if (!State.PrevFrameBrightness.IsValid())
		{
			State.CurrentReprojectedBloom = nullptr;
			State.CurrentPyramidBlurred = nullptr;
			State.CurrentDebugExtractRDG = nullptr;
			return;
		}

		RDG_GPU_STAT_SCOPE(GraphBuilder, MobileSubpassBloom);
		RDG_EVENT_SCOPE(GraphBuilder, "MobileSubpassBloom.PreBasepassPyramid");

		FRDGTextureRef BrightnessRDG = GraphBuilder.RegisterExternalTexture(State.PrevFrameBrightness, TEXT("MobileSubpassBloom.PrevBrightness"));

		IConsoleVariable* PyramidDepthCVar = GetSubpassBloomPyramidDepthCVar();
		const int32 PyramidDepth = PyramidDepthCVar != nullptr ? FMath::Clamp(PyramidDepthCVar->GetInt(), 1, 6) : 6;
		FRDGTextureRef Blurred = BuildDualFilterPyramid_RenderThread(GraphBuilder, View.GetFeatureLevel(), BrightnessRDG, PyramidDepth, View.FinalPostProcessSettings);

		FRDGTextureRef Reprojected = ReprojectBloom_RenderThread(
			GraphBuilder,
			View,
			Blurred,
			MotionVectorTexture,
			DepthTexture,
			PrevViewProjMatrices);

		State.CurrentReprojectedBloom = Reprojected;
		// Keep current-frame RDG handles for provider debug views; clear them next frame.
		State.CurrentPyramidBlurred = Blurred;
		State.CurrentDebugExtractRDG = BrightnessRDG;
	}

	void FMobileSubpassBloom::ProvideMobileSubpassPostProcessInputs(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		EMobileSubpassPostProcessSlot Slot,
		FMobileSubpassPostProcessInputs& Inout)
	{
		if (Slot != EMobileSubpassPostProcessSlot::Bloom || !IsExecutionEnabled())
		{
			return;
		}
		const int32 ViewIdx = GetViewStateIndex(View);
		FPerViewState& State = ViewStates[ViewIdx];
		if (State.CurrentReprojectedBloom == nullptr)
		{
			return;
		}

		Inout.BloomTexture = State.CurrentReprojectedBloom;
		const FPostProcessSettings& PPS = View.FinalPostProcessSettings;
		Inout.BloomIntensity = FMath::Max(0.0f, PPS.BloomIntensity) * RenderSettings.IntensityScale;
		// Per-mip bloom tint is already applied in the up chain; applying it here would double-tint.
		Inout.BloomTint = FVector4f(RenderSettings.TintScale.R, RenderSettings.TintScale.G, RenderSettings.TintScale.B, 1.0f);

		// Match stock tonemapper scaling so IntensityScale also affects dirt-mask strength.
		if (PPS.BloomDirtMask
			&& PPS.BloomDirtMask->GetResource()
			&& PPS.BloomDirtMask->GetResource()->TextureRHI.IsValid()
			&& PPS.BloomDirtMaskIntensity > 0.0f
			&& PPS.BloomIntensity > 0.0f)
		{
			Inout.DirtMaskTexture = RegisterExternalTexture(
				GraphBuilder,
				PPS.BloomDirtMask->GetResource()->TextureRHI,
				TEXT("MobileSubpassBloom.DirtMask"));
			const float Scale = PPS.BloomDirtMaskIntensity / PPS.BloomIntensity;
			Inout.DirtMaskTint = FVector4f(
				PPS.BloomDirtMaskTint.R * Scale,
				PPS.BloomDirtMaskTint.G * Scale,
				PPS.BloomDirtMaskTint.B * Scale,
				1.0f);
		}

		Inout.DebugExtractTexture = State.CurrentDebugExtractRDG;
		Inout.DebugPyramidTexture = State.CurrentPyramidBlurred;
	}
} // namespace OculusXR

#endif // WITH_OCULUS_BRANCH
