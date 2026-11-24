// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "MobileSubpassBloomComponent.h"

#include "Engine/World.h"
#include "RenderingThread.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#ifdef WITH_OCULUS_BRANCH
#include "MobileSubpassBloom.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogMobileSubpassBloomComponent, Log, All);

// Everything the effect needs -- the provider interface, the two scene-view-extension
// hooks and IsMobileSubpassBloomEnabledAtInit() -- ships only in the Meta engine fork,
// so the implementation is gated wholesale and the #else branch below keeps the
// component linkable on a stock engine. UMobileSubpassBloomComponent itself cannot be
// gated: UHT skips the body of any #ifdef it does not recognise, and it does not
// recognise WITH_OCULUS_BRANCH, so gating the UCLASS would strip its reflection data
// from the fork build as well and break the configuration the effect actually ships in.
#ifdef WITH_OCULUS_BRANCH

namespace
{
	struct FMobileSubpassBloomWorldEntry
	{
		const UWorld* WorldAddress = nullptr;
		TWeakObjectPtr<UMobileSubpassBloomComponent> ActiveComponent;
		uint64 OwnerId = 0;
	};

	TMap<TWeakObjectPtr<UWorld>, FMobileSubpassBloomWorldEntry> GComponentRegistry_GameThread;
	uint64 GNextActivationOwnerId_GameThread = 1;
	FDelegateHandle GWorldCleanupHandle_GameThread;

	TMap<const UWorld*, uint64> GActiveOwners_RenderThread;

	void OnMobileSubpassBloomWorldCleanup_GameThread(
		UWorld* World,
		bool,
		bool)
	{
		check(IsInGameThread());
		if (FMobileSubpassBloomWorldEntry* Entry = World ? GComponentRegistry_GameThread.Find(World) : nullptr)
		{
			// A world address may be reused before this runs; remove only our owner.
			const uint64 OwnerId = Entry->OwnerId;
			ENQUEUE_RENDER_COMMAND(RemoveMobileSubpassBloomComponentWorld)(
				[World, OwnerId](FRHICommandListImmediate&) {
					check(IsInRenderingThread());
					if (GActiveOwners_RenderThread.FindRef(World) == OwnerId)
					{
						GActiveOwners_RenderThread.Remove(World);
					}
				});
			GComponentRegistry_GameThread.Remove(World);
		}
	}

	const UWorld* GetViewWorld_RenderThread(const FSceneView& View)
	{
		check(IsInRenderingThread());
		// Render-thread code uses the world address only as an identity token.
		return View.Family && View.Family->Scene
			? View.Family->Scene->GetWorld()
			: nullptr;
	}

	OculusXR::FMobileSubpassBloomRenderSettings MakeRenderSettings(
		const UMobileSubpassBloomComponent& Component)
	{
		check(IsInGameThread());
		OculusXR::FMobileSubpassBloomRenderSettings Settings;
		Settings.IntensityScale = FMath::Max(0.0f, Component.IntensityScale);
		Settings.TintScale = FLinearColor(
			FMath::Max(0.0f, Component.TintScale.R),
			FMath::Max(0.0f, Component.TintScale.G),
			FMath::Max(0.0f, Component.TintScale.B),
			Component.TintScale.A);
		Settings.bSunMerge = Component.bSunMerge;
		Settings.SunSkyStartCm = FMath::Max(0.0f, Component.SunSkyStartCm);
		Settings.SunSkyEndCm = FMath::Max(0.0f, Component.SunSkyEndCm);
		if (Settings.bSunMerge && Settings.SunSkyEndCm <= Settings.SunSkyStartCm)
		{
			static bool bWarnedSunSkyBand = false;
			if (!bWarnedSunSkyBand)
			{
				UE_LOG(LogMobileSubpassBloomComponent, Warning,
					TEXT("Mobile Subpass Bloom: SunSkyStartCm (%.1f) >= SunSkyEndCm (%.1f) -- the sun-merge sky band is empty or inverted and will collapse to a 1cm hard step at render. Set SunSkyEndCm > SunSkyStartCm."),
					Settings.SunSkyStartCm, Settings.SunSkyEndCm);
				bWarnedSunSkyBand = true;
			}
		}
		Settings.SunIntensity = FMath::Max(0.0f, Component.SunIntensity);
		Settings.SunHaloWeight = FMath::Max(0.0f, Component.SunHaloWeight);
		Settings.SunCoreTightness = FMath::Max(0.0f, Component.SunCoreTightness);
		Settings.SunHaloFalloff = FMath::Max(0.0f, Component.SunHaloFalloff);
		return Settings;
	}

	class FMobileSubpassBloomViewExtension final : public FWorldSceneViewExtension
	{
	public:
		FMobileSubpassBloomViewExtension(
			const FAutoRegister& AutoRegister,
			UWorld* World,
			uint64 InOwnerId,
			const UMobileSubpassBloomComponent* InOwnerComponent)
			: FWorldSceneViewExtension(AutoRegister, World)
			, OwnerId(InOwnerId)
			, OwnerComponent(InOwnerComponent)
		{
		}

		// Snapshot on the game thread before this family's render hooks.
		virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

		virtual void RegisterMobileSubpassPostProcessProviders(
			FRDGBuilder& GraphBuilder,
			const FSceneView& InView,
			FMobileSubpassPostProcessProviderArray& OutProviders) override;

		virtual void PostInlineTonemapSubpass_RenderThread(
			FRDGBuilder& GraphBuilder,
			const FSceneView& InView,
			FRDGTextureRef TonemapOutputTexture,
			FRDGTextureRef LuminanceMap,
			FRDGTextureRef SceneDepthTexture) override;

		void SetRenderSettings_RenderThread(
			const OculusXR::FMobileSubpassBloomRenderSettings& InSettings)
		{
			check(IsInRenderingThread());
			Bloom.SetRenderSettings_RenderThread(InSettings);
		}

	protected:
		virtual bool IsActiveThisFrame_Internal(
			const FSceneViewExtensionContext& Context) const override;

	private:
		bool IsSupportedOwnerView_RenderThread(const FSceneView& View) const;
		bool IsSupportedFamily_RenderThread(const FSceneViewFamily& Family) const;
		bool IsSupportedView_RenderThread(const FSceneView& View) const;

		OculusXR::FMobileSubpassBloom Bloom;
		const uint64 OwnerId;
		const TWeakObjectPtr<const UMobileSubpassBloomComponent> OwnerComponent;
		mutable bool bWarnedUnsupportedViewTopology = false;
	};

	bool FMobileSubpassBloomViewExtension::IsActiveThisFrame_Internal(
		const FSceneViewExtensionContext& Context) const
	{
		UWorld* ContextWorld = Context.GetWorld();
		return FWorldSceneViewExtension::IsActiveThisFrame_Internal(Context)
			&& ContextWorld
			&& ContextWorld->IsGameWorld();
	}

	void FMobileSubpassBloomViewExtension::BeginRenderViewFamily(FSceneViewFamily&)
	{
		check(IsInGameThread());
		const UMobileSubpassBloomComponent* Component = OwnerComponent.Get();
		if (!Component)
		{
			return;
		}
		const OculusXR::FMobileSubpassBloomRenderSettings Settings = MakeRenderSettings(*Component);
		const TSharedRef<FMobileSubpassBloomViewExtension, ESPMode::ThreadSafe> Self = SharedThis(this);
		ENQUEUE_RENDER_COMMAND(SetMobileSubpassBloomRenderSettings)(
			[Self, Settings](FRHICommandListImmediate&) {
				Self->SetRenderSettings_RenderThread(Settings);
			});
	}

	bool FMobileSubpassBloomViewExtension::IsSupportedView_RenderThread(
		const FSceneView& View) const
	{
		check(IsInRenderingThread());
		return View.bIsGameView
			&& !View.bIsSceneCapture
			&& !View.bIsSceneCaptureCube
			&& !View.bIsReflectionCapture
			&& !View.bIsPlanarReflection;
	}

	bool FMobileSubpassBloomViewExtension::IsSupportedFamily_RenderThread(
		const FSceneViewFamily& Family) const
	{
		check(IsInRenderingThread());
		if (Family.Views.IsEmpty() || Family.bThumbnailRendering)
		{
			return false;
		}

		const FSceneView* FirstView = Family.Views[0];
		if (!FirstView || !IsSupportedView_RenderThread(*FirstView))
		{
			return false;
		}

		// Only mono and single-pass stereo are supported; array slices must map to
		// StereoViewIndex 0 and 1.
		const int32 NumViews = Family.Views.Num();
		const bool bSupportedTopology = NumViews == 1
			|| (NumViews == 2
				&& Family.Views[0] && Family.Views[1]
				&& Family.Views[0]->bIsSinglePassStereo
				&& Family.Views[0]->StereoViewIndex == 0
				&& Family.Views[1]->StereoViewIndex == 1);
		if (!bSupportedTopology)
		{
			if (!bWarnedUnsupportedViewTopology)
			{
				UE_LOG(LogMobileSubpassBloomComponent, Warning,
					TEXT("Mobile Subpass Bloom supports mono or single-pass stereo (slices 0 and 1); this view family will no-op."));
				bWarnedUnsupportedViewTopology = true;
			}
			return false;
		}

		return true;
	}

	bool FMobileSubpassBloomViewExtension::IsSupportedOwnerView_RenderThread(
		const FSceneView& View) const
	{
		check(IsInRenderingThread());
		return IsMobileSubpassBloomEnabledAtInit()
			&& OculusXR::IsMobileSubpassBloomComponentOwner_RenderThread(View, OwnerId)
			&& View.Family
			&& IsSupportedFamily_RenderThread(*View.Family)
			&& IsSupportedView_RenderThread(View);
	}

	void FMobileSubpassBloomViewExtension::RegisterMobileSubpassPostProcessProviders(
		FRDGBuilder& GraphBuilder,
		const FSceneView& InView,
		FMobileSubpassPostProcessProviderArray& OutProviders)
	{
		check(IsInRenderingThread());
		if (!IsSupportedOwnerView_RenderThread(InView))
		{
			return;
		}

		// Motion vectors are not plumbed; reprojection uses the matrix (ClipToPrevClip) path.
		FRHITexture* MotionVectorRHI = nullptr;
		FRHITexture* DepthRHI = Bloom.GetPrevFrameSceneDepthRHI(InView);
		Bloom.BuildPyramidAndReproject_RenderThread(
			GraphBuilder, InView, MotionVectorRHI, DepthRHI);
		OutProviders.Add(&Bloom);
	}

	void FMobileSubpassBloomViewExtension::PostInlineTonemapSubpass_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& InView,
		FRDGTextureRef TonemapOutputTexture,
		FRDGTextureRef LuminanceMap,
		FRDGTextureRef SceneDepthTexture)
	{
		check(IsInRenderingThread());
		if (!IsSupportedOwnerView_RenderThread(InView))
		{
			return;
		}

		Bloom.StashSceneDepthForNextFrame_RenderThread(
			GraphBuilder, InView, SceneDepthTexture);
		const bool bSourceIsLDR = (LuminanceMap == nullptr);
		FRDGTextureRef ExtractSource = LuminanceMap
			? LuminanceMap
			: TonemapOutputTexture;
		Bloom.ExtractBrightnessForNextFrame_RenderThread(
			GraphBuilder, InView, ExtractSource, bSourceIsLDR);
	}
} // namespace

void OculusXR::InitializeMobileSubpassBloomComponentRegistry_GameThread()
{
	check(IsInGameThread());
	if (!GWorldCleanupHandle_GameThread.IsValid())
	{
		GWorldCleanupHandle_GameThread = FWorldDelegates::OnWorldCleanup.AddStatic(
			&OnMobileSubpassBloomWorldCleanup_GameThread);
	}
}

void OculusXR::ShutdownMobileSubpassBloomComponentRegistry_GameThread()
{
	check(IsInGameThread());
	if (GWorldCleanupHandle_GameThread.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(GWorldCleanupHandle_GameThread);
		GWorldCleanupHandle_GameThread.Reset();
	}
	GComponentRegistry_GameThread.Reset();

	// The render-thread map must be cleared too. Owner ids restart at 1 when this
	// module is reloaded, so a stale entry left behind here can be matched by a
	// reused world address plus a recycled id, driving bloom for a non-owner.
	// Flush rather than fire-and-forget: this lambda's code is unloaded with the module.
	ENQUEUE_RENDER_COMMAND(ClearMobileSubpassBloomComponentOwners)(
		[](FRHICommandListImmediate&) {
			check(IsInRenderingThread());
			GActiveOwners_RenderThread.Reset();
		});
	FlushRenderingCommands();
}

void OculusXR::SetMobileSubpassBloomComponentOwner_GameThread(
	const UWorld* World,
	uint64 OwnerId,
	bool bActive)
{
	check(IsInGameThread());
	if (!World || OwnerId == 0)
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(SetMobileSubpassBloomComponentOwner)(
		[World, OwnerId, bActive](FRHICommandListImmediate&) {
			check(IsInRenderingThread());
			if (bActive)
			{
				GActiveOwners_RenderThread.Add(World, OwnerId);
			}
			else if (GActiveOwners_RenderThread.FindRef(World) == OwnerId)
			{
				GActiveOwners_RenderThread.Remove(World);
			}
		});
}

bool OculusXR::IsMobileSubpassBloomComponentOwner_RenderThread(
	const FSceneView& View,
	uint64 OwnerId)
{
	check(IsInRenderingThread());
	return OwnerId != 0
		&& GActiveOwners_RenderThread.FindRef(GetViewWorld_RenderThread(View)) == OwnerId;
}

UMobileSubpassBloomComponent::UMobileSubpassBloomComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

bool UMobileSubpassBloomComponent::RegisterComponentWorld()
{
	check(IsInGameThread());
	UWorld* World = GetWorld();
	if (!IsRegistered() || !World || !World->IsGameWorld())
	{
		return false;
	}
	if (RegisteredWorld.Get() == World)
	{
		return true;
	}

	FMobileSubpassBloomWorldEntry& Entry = GComponentRegistry_GameThread.FindOrAdd(World);
	Entry.WorldAddress = World;
	RegisteredWorld = World;
	return true;
}

void UMobileSubpassBloomComponent::UnregisterComponentWorld()
{
	check(IsInGameThread());
	const TWeakObjectPtr<UWorld> WorldKey = RegisteredWorld;
	if (FMobileSubpassBloomWorldEntry* Entry = GComponentRegistry_GameThread.Find(WorldKey))
	{
		if (Entry->ActiveComponent.Get() == this)
		{
			Entry->ActiveComponent.Reset();
			Entry->OwnerId = 0;
		}
		if (!Entry->ActiveComponent.IsValid())
		{
			GComponentRegistry_GameThread.Remove(WorldKey);
		}
	}
	RegisteredWorld.Reset();
}

void UMobileSubpassBloomComponent::WarnApparatusDisabledOnce()
{
	check(IsInGameThread());
	if (!bWarnedApparatusDisabled)
	{
		UE_LOG(LogMobileSubpassBloomComponent, Warning,
			TEXT("Mobile Subpass Bloom cannot activate because r.Mobile.SubpassBloom was not 1 when the launch gate was latched. Set r.Mobile.SubpassBloom=1 in DefaultEngine.ini [SystemSettings] and relaunch; runtime changes are ignored."));
		bWarnedApparatusDisabled = true;
	}
}

void UMobileSubpassBloomComponent::OnRegister()
{
	Super::OnRegister();
	if (!IsMobileSubpassBloomEnabledAtInit())
	{
		WarnApparatusDisabledOnce();
	}
}

void UMobileSubpassBloomComponent::Activate(bool bReset)
{
	check(IsInGameThread());
	if (!IsMobileSubpassBloomEnabledAtInit())
	{
		WarnApparatusDisabledOnce();
		SetActiveFlag(false);
		return;
	}
	if (!RegisterComponentWorld())
	{
		SetActiveFlag(false);
		return;
	}
	if (ViewExtension.IsValid())
	{
		Super::Activate(bReset);
		return;
	}

	UWorld* World = RegisteredWorld.Get();
	FMobileSubpassBloomWorldEntry& Entry = GComponentRegistry_GameThread.FindChecked(World);
	if (UMobileSubpassBloomComponent* ActiveComponent = Entry.ActiveComponent.Get())
	{
		if (ActiveComponent != this)
		{
			UE_LOG(LogMobileSubpassBloomComponent, Warning,
				TEXT("Mobile Subpass Bloom component %s cannot activate because %s already owns this world."),
				*GetPathName(), *ActiveComponent->GetPathName());
			UnregisterComponentWorld();
			SetActiveFlag(false);
			return;
		}
	}

	do
	{
		ActivationOwnerId = GNextActivationOwnerId_GameThread++;
	}
	while (ActivationOwnerId == 0);
	Entry.ActiveComponent = this;
	Entry.OwnerId = ActivationOwnerId;
	ViewExtension = FSceneViewExtensions::NewExtension<
		FMobileSubpassBloomViewExtension>(World, ActivationOwnerId, this);
	OculusXR::SetMobileSubpassBloomComponentOwner_GameThread(
		World, ActivationOwnerId, true);
	Super::Activate(bReset);
}

void UMobileSubpassBloomComponent::ReleaseActiveOwner()
{
	check(IsInGameThread());
	const TWeakObjectPtr<UWorld> WorldKey = RegisteredWorld;
	const UWorld* WorldAddress = nullptr;
	const uint64 OwnerId = ActivationOwnerId;
	if (FMobileSubpassBloomWorldEntry* Entry = GComponentRegistry_GameThread.Find(WorldKey))
	{
		WorldAddress = Entry->WorldAddress;
		if (Entry->ActiveComponent.Get() == this)
		{
			Entry->ActiveComponent.Reset();
			Entry->OwnerId = 0;
		}
	}
	OculusXR::SetMobileSubpassBloomComponentOwner_GameThread(
		WorldAddress, OwnerId, false);
	ViewExtension.Reset();
	ActivationOwnerId = 0;
}

void UMobileSubpassBloomComponent::Deactivate()
{
	ReleaseActiveOwner();
	UnregisterComponentWorld();
	Super::Deactivate();
}

void UMobileSubpassBloomComponent::OnUnregister()
{
	Deactivate();
	Super::OnUnregister();
}

#else // WITH_OCULUS_BRANCH

UMobileSubpassBloomComponent::UMobileSubpassBloomComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UMobileSubpassBloomComponent::WarnApparatusDisabledOnce()
{
	check(IsInGameThread());
	if (!bWarnedApparatusDisabled)
	{
		UE_LOG(LogMobileSubpassBloomComponent, Warning,
			TEXT("Mobile Subpass Bloom cannot activate: the effect renders inside the mobile tonemap subpass, which only exists in the Meta fork of Unreal Engine. Build against the Meta engine fork to use this component."));
		bWarnedApparatusDisabled = true;
	}
}

void UMobileSubpassBloomComponent::OnRegister()
{
	Super::OnRegister();
	WarnApparatusDisabledOnce();
}

void UMobileSubpassBloomComponent::Activate(bool)
{
	WarnApparatusDisabledOnce();
	SetActiveFlag(false);
}

void UMobileSubpassBloomComponent::Deactivate()
{
	ReleaseActiveOwner();
	UnregisterComponentWorld();
	Super::Deactivate();
}

void UMobileSubpassBloomComponent::OnUnregister()
{
	Deactivate();
	Super::OnUnregister();
}

bool UMobileSubpassBloomComponent::RegisterComponentWorld()
{
	return false;
}

void UMobileSubpassBloomComponent::UnregisterComponentWorld()
{
	RegisteredWorld.Reset();
}

void UMobileSubpassBloomComponent::ReleaseActiveOwner()
{
	ViewExtension.Reset();
	ActivationOwnerId = 0;
}

#endif // WITH_OCULUS_BRANCH
