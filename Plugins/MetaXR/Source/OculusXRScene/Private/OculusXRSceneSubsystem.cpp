// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRSceneSubsystem.h"
#include "OculusXRSceneTypes.h"
#include "OculusXRScene.h"
#include "IOculusXRSceneModule.h"
#include "OculusXRSceneDelegates.h"
#include "OculusXRAnchorBPFunctionLibrary.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OculusXRSceneModule.h"
#include "OculusXRPassthroughModule.h"
#include "OculusXRPassthroughXR.h"
#include "OpenXR/OculusXROpenXRUtilities.h"

UOculusXRSceneSubsystem::UOculusXRSceneSubsystem()
	: requestedVisibilityState_(EOculusXRBoundaryVisibility::NotSuppressed)
	, bInitialized(false)
{
}

bool UOculusXRSceneSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GetDefault<UOculusXRHMDRuntimeSettings>()->bBoundaryVisibilitySupportEnabled && GetDefault<UOculusXRHMDRuntimeSettings>()->bInsightPassthroughEnabled;
}

void UOculusXRSceneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	bool enabled = GetDefault<UOculusXRHMDRuntimeSettings>()->bBoundaryVisibilitySupportEnabled;
	bool suppressed = GetDefault<UOculusXRHMDRuntimeSettings>()->bDefaultBoundaryVisibilitySuppressed;

	// This is the desired state
	bool isBoundaryNotSuppressed = enabled && !suppressed;
	requestedVisibilityState_ = isBoundaryNotSuppressed ? EOculusXRBoundaryVisibility::NotSuppressed : EOculusXRBoundaryVisibility::Suppressed;
	visChangedEventHandle_ = FOculusXRSceneEventDelegates::OculusBoundaryVisibilityChanged.AddUObject(this, &UOculusXRSceneSubsystem::OnBoundaryVisibilityChanged);

	bInitialized = true;
}

void UOculusXRSceneSubsystem::Deinitialize()
{
	FOculusXRSceneEventDelegates::OculusBoundaryVisibilityChanged.Remove(visChangedEventHandle_);
	bInitialized = false;
}

ETickableTickType UOculusXRSceneSubsystem::GetTickableTickType() const
{
	return IsTemplate() ? ETickableTickType::Never : FTickableGameObject::GetTickableTickType();
}

bool UOculusXRSceneSubsystem::IsAllowedToTick() const
{
	return !IsTemplate() && bInitialized;
}

void UOculusXRSceneSubsystem::Tick(float DeltaTime)
{
	UpdateBoundary();
}

EOculusXRBoundaryVisibility UOculusXRSceneSubsystem::GetBoundaryVisibility()
{
	EOculusXRBoundaryVisibility boundaryVisibility = {};
	OculusXRScene::FOculusXRScene::GetBoundaryVisibility(boundaryVisibility);
	return boundaryVisibility;
}

EOculusXRBoundaryVisibility UOculusXRSceneSubsystem::GetRequestedBoundaryVisibility()
{
	return requestedVisibilityState_;
}

void UOculusXRSceneSubsystem::SetRequestedBoundaryVisibility(EOculusXRBoundaryVisibility Visibility)
{
	requestedVisibilityState_ = Visibility;
}

void UOculusXRSceneSubsystem::OnBoundaryVisibilityChanged(EOculusXRBoundaryVisibility visibility)
{
	// Do nothing on event
}

void UOculusXRSceneSubsystem::UpdateBoundary()
{
	// If the state is the same, skip
	auto currentVisibilityState = GetBoundaryVisibility();
	if (currentVisibilityState == requestedVisibilityState_)
	{
		return;
	}

	// TODO: This should probably be part of the passthrough API
	if (OculusXR::IsOpenXRSystem())
	{
		if (!FOculusXRPassthroughModule::Get().GetPassthroughExtensionPlugin().Pin()->IsPassthroughEnabled())
		{
			return;
		}
	}

	OculusXRScene::FOculusXRScene::RequestBoundaryVisibility(requestedVisibilityState_);
}
