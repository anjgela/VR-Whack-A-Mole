// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRMovement.h"
#include "OculusXRMovementLog.h"
#include "OculusXRMovementModule.h"
#include "OculusXRHMDPrivate.h"
#include "OculusXRMovementFunctionsOpenXR.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "OculusXRMovement"

bool OculusXRMovement::GetBodyState(FOculusXRBodyState& outOculusXRBodyState)
{
	return GetOculusXRMovementFunctionsImpl()->GetBodyState(outOculusXRBodyState);
}

bool OculusXRMovement::GetBodySkeleton(FOculusXRBodySkeleton& outOculusXRBodyState)
{
	return GetOculusXRMovementFunctionsImpl()->GetBodySkeleton(outOculusXRBodyState);
}

bool OculusXRMovement::IsBodyTrackingEnabled()
{
	return GetOculusXRMovementFunctionsImpl()->IsBodyTrackingEnabled();
}

bool OculusXRMovement::IsBodyTrackingSupported()
{
	return GetOculusXRMovementFunctionsImpl()->IsBodyTrackingSupported();
}

bool OculusXRMovement::RequestBodyTrackingFidelity(EOculusXRBodyTrackingFidelity fidelity)
{
	return GetOculusXRMovementFunctionsImpl()->RequestBodyTrackingFidelity(fidelity);
}

bool OculusXRMovement::ResetBodyTrackingCalibration()
{
	return GetOculusXRMovementFunctionsImpl()->ResetBodyTrackingCalibration();
}

bool OculusXRMovement::SuggestBodyTrackingCalibrationOverride(float height)
{
	return GetOculusXRMovementFunctionsImpl()->SuggestBodyTrackingCalibrationOverride(height);
}

bool OculusXRMovement::StartBodyTracking()
{
	return GetOculusXRMovementFunctionsImpl()->StartBodyTracking();
}

bool OculusXRMovement::StartBodyTrackingByJointSet(EOculusXRBodyJointSet jointSet)
{
	return GetOculusXRMovementFunctionsImpl()->StartBodyTrackingByJointSet(jointSet);
}

bool OculusXRMovement::StopBodyTracking()
{
	return GetOculusXRMovementFunctionsImpl()->StopBodyTracking();
}

bool OculusXRMovement::GetFaceState(FOculusXRFaceState& outOculusXRFaceState)
{
	return GetOculusXRMovementFunctionsImpl()->GetFaceState(outOculusXRFaceState);
}

bool OculusXRMovement::IsFaceTrackingEnabled()
{
	return GetOculusXRMovementFunctionsImpl()->IsFaceTrackingEnabled();
}

bool OculusXRMovement::IsFaceTrackingSupported()
{
	return GetOculusXRMovementFunctionsImpl()->IsFaceTrackingSupported();
}

bool OculusXRMovement::IsFaceTrackingVisemesEnabled()
{
	return GetOculusXRMovementFunctionsImpl()->IsFaceTrackingVisemesEnabled();
}

bool OculusXRMovement::IsFaceTrackingVisemesSupported()
{
	return GetOculusXRMovementFunctionsImpl()->IsFaceTrackingVisemesSupported();
}

bool OculusXRMovement::SetFaceTrackingVisemesEnabled(bool enabled)
{
	return GetOculusXRMovementFunctionsImpl()->SetFaceTrackingVisemesEnabled(enabled);
}

bool OculusXRMovement::GetFaceVisemesState(FOculusXRFaceVisemesState& outOculusXRFaceVisemesState)
{
	return GetOculusXRMovementFunctionsImpl()->GetFaceVisemesState(outOculusXRFaceVisemesState);
}

bool OculusXRMovement::StartFaceTracking()
{
	return GetOculusXRMovementFunctionsImpl()->StartFaceTracking();
}

bool OculusXRMovement::StopFaceTracking()
{
	return GetOculusXRMovementFunctionsImpl()->StopFaceTracking();
}

bool OculusXRMovement::GetEyeGazesState(FOculusXREyeGazesState& outOculusXREyeGazesState)
{
	return GetOculusXRMovementFunctionsImpl()->GetEyeGazesState(outOculusXREyeGazesState);
}

bool OculusXRMovement::IsEyeTrackingEnabled()
{
	return GetOculusXRMovementFunctionsImpl()->IsEyeTrackingEnabled();
}

bool OculusXRMovement::IsEyeTrackingSupported()
{
	return GetOculusXRMovementFunctionsImpl()->IsEyeTrackingSupported();
}

bool OculusXRMovement::StartEyeTracking()
{
	return GetOculusXRMovementFunctionsImpl()->StartEyeTracking();
}

bool OculusXRMovement::StopEyeTracking()
{
	return GetOculusXRMovementFunctionsImpl()->StopEyeTracking();
}

bool OculusXRMovement::IsFullBodyTrackingEnabled()
{
	return GetOculusXRMovementFunctionsImpl()->IsFullBodyTrackingEnabled();
}

TSharedPtr<IOculusXRMovementFunctions> OculusXRMovement::MovementFunctionsImpl = nullptr;
TSharedPtr<IOculusXRMovementFunctions> OculusXRMovement::GetOculusXRMovementFunctionsImpl()
{
	if (MovementFunctionsImpl == nullptr)
	{
		MovementFunctionsImpl = MakeShared<FOculusXRMovementFunctionsOpenXR>();
	}

	return MovementFunctionsImpl;
}

#undef LOCTEXT_NAMESPACE
