// Copyright (c) Meta Platforms, Inc. and affiliates.

// Deterministic host/automation test for D112265901: the haptics extension's
// GetOpenXRInstance()/GetOpenXRSession() must fetch the LIVE handles from the
// current OpenXR HMD each call, not return a handle cached once in
// PostCreateSession. Over PC Link the XrSession is recreated; a cached-once
// handle goes stale (no vibration / bad handle), which the fix eliminates by
// delegating to GEngine->XRSystem->GetIOpenXRHMD(). No device required — a mock
// OpenXR HMD/tracking system stands in for the runtime.

#if WITH_DEV_AUTOMATION_TESTS

#include "OculusXRInputHapticsExtensionPlugin.h"

#include "Engine/Engine.h"
#include "IOpenXRHMD.h"
#include "IXRTrackingSystem.h"
#include "Misc/AutomationTest.h"
#include "XRTrackingSystemBase.h"

namespace OculusXRInput
{
	namespace
	{
		// Mock OpenXR HMD whose instance/session handles are controllable, so the
		// test can simulate a session recreation (S1 -> S2). Only GetInstance()/
		// GetSession() carry behaviour; the rest are inert stubs.
		class FMockOpenXRHMD : public IOpenXRHMD
		{
		public:
			XrInstance MockInstance = XR_NULL_HANDLE;
			XrSession MockSession = XR_NULL_HANDLE;

			virtual XrInstance GetInstance() override
			{
				return MockInstance;
			}
			virtual XrSession GetSession() override
			{
				return MockSession;
			}

			// --- inert stubs for the rest of IOpenXRHMD -----------------------
			virtual void SetInputModule(class IOpenXRInputModule*) override {}
			virtual bool IsInitialized() const override
			{
				return true;
			}
			virtual bool IsRunning() const override
			{
				return true;
			}
			virtual bool IsFocused() const override
			{
				return true;
			}
			virtual int32 AddTrackedDevice(XrAction, XrPath) override
			{
				return 0;
			}
			virtual int32 AddTrackedDevice(XrAction, XrPath, XrPath) override
			{
				return 0;
			}
			virtual void ResetTrackedDevices() override {}
			virtual XrPath GetTrackedDevicePath(const int32) override
			{
				return XR_NULL_PATH;
			}
			virtual XrSpace GetTrackedDeviceSpace(const int32) override
			{
				return XR_NULL_HANDLE;
			}
			virtual bool IsExtensionEnabled(const FString&) const override
			{
				return false;
			}
			virtual bool IsOpenXRAPIVersionMet(EOpenXRAPIVersion) const override
			{
				return true;
			}
			virtual XrSystemId GetSystem() override
			{
				return XR_NULL_SYSTEM_ID;
			}
			virtual XrTime GetDisplayTime() const override
			{
				return 0;
			}
			virtual XrSpace GetTrackingSpace() const override
			{
				return XR_NULL_HANDLE;
			}
			virtual class IOpenXRExtensionPluginDelegates& GetIOpenXRExtensionPluginDelegates() override
			{
				// Never called by the getters under test.
				checkNoEntry();
				return *reinterpret_cast<IOpenXRExtensionPluginDelegates*>(this);
			}
			virtual bool GetIsTracked(int32) override
			{
				return false;
			}
			virtual bool GetPoseForTime(int32, FTimespan, bool&, FQuat&, FVector&, bool&, FVector&, bool&, FVector&, bool&, FVector&, float) override
			{
				return false;
			}
			virtual TArray<class IOpenXRExtensionPlugin*>& GetExtensionPlugins() override
			{
				static TArray<IOpenXRExtensionPlugin*> Empty;
				return Empty;
			}
			virtual void SetEnvironmentBlendMode(XrEnvironmentBlendMode) override {}
			virtual XrEnvironmentBlendMode GetEnvironmentBlendMode() override
			{
				return XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
			}
			virtual TArray<XrEnvironmentBlendMode> GetSupportedEnvironmentBlendModes() const override
			{
				return {};
			}
			virtual void GetFullFOVInformation(TArray<XrFovf>&) const override {}
			virtual bool AllocateSwapchainTextures_RenderThread(const FOpenXRSwapchainProperties&, FXRSwapChainPtr&, uint8&) override
			{
				return false;
			}
#ifdef WITH_OCULUS_BRANCH
			// IOpenXRHMD declares these only on the Meta engine fork (headlock support).
			virtual void SetTrackingPositionEnabled(const int32, bool) override {}
			virtual void SetTrackingOrientationEnabled(const int32, bool) override {}
#endif // WITH_OCULUS_BRANCH
		};

		// Mock tracking system that reports as "OpenXR" and hands out the mock HMD
		// — exactly what the plugin's anonymous GetOpenXRHMD() helper checks
		// (GEngine->XRSystem->GetSystemName() == "OpenXR" then GetIOpenXRHMD()).
		// FXRTrackingSystemBase supplies defaults for the rest of IXRTrackingSystem.
		class FMockOpenXRTrackingSystem : public FXRTrackingSystemBase
		{
		public:
			FMockOpenXRTrackingSystem()
				: FXRTrackingSystemBase(nullptr)
			{
			}

			FMockOpenXRHMD Hmd;

			virtual FName GetSystemName() const override
			{
				static const FName Name(TEXT("OpenXR"));
				return Name;
			}
			virtual IOpenXRHMD* GetIOpenXRHMD() override
			{
				return &Hmd;
			}

			// --- remaining IXRTrackingSystem pure virtuals: inert stubs --------
			virtual int32 GetXRSystemFlags() const override
			{
				return 0;
			}
			virtual bool EnumerateTrackedDevices(TArray<int32>&, EXRTrackedDeviceType) override
			{
				return false;
			}
			virtual bool GetCurrentPose(int32, FQuat&, FVector&) override
			{
				return false;
			}
			virtual float GetWorldToMetersScale() const override
			{
				return 100.0f;
			}
			virtual void ResetOrientationAndPosition(float) override {}
			virtual bool IsHeadTrackingAllowed() const override
			{
				return false;
			}
		};

		XrInstance MakeInstance(uint64 V)
		{
			return reinterpret_cast<XrInstance>(static_cast<uintptr_t>(V));
		}
		XrSession MakeSession(uint64 V)
		{
			return reinterpret_cast<XrSession>(static_cast<uintptr_t>(V));
		}
	} // namespace

#if UE_VERSION_OLDER_THAN(5, 5, 0)
	BEGIN_DEFINE_SPEC(
		FOculusXRInputHapticsGettersSpec,
		"OculusXR.Input.HapticsGetters",
		EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
	END_DEFINE_SPEC(FOculusXRInputHapticsGettersSpec)
#else
	BEGIN_DEFINE_SPEC(
		FOculusXRInputHapticsGettersSpec,
		"OculusXR.Input.HapticsGetters",
		EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
	END_DEFINE_SPEC(FOculusXRInputHapticsGettersSpec)
#endif

	void FOculusXRInputHapticsGettersSpec::Define()
	{
		Describe("GetOpenXRInstance/GetOpenXRSession (D112265901)", [this]() {
			It("returns live handles and reflects a session recreation, not a stale cached one", [this]() {
				if (!TestNotNull(TEXT("GEngine"), GEngine))
				{
					return;
				}

				TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> PrevXRSystem = GEngine->XRSystem;
				TSharedRef<FMockOpenXRTrackingSystem, ESPMode::ThreadSafe> Mock =
					MakeShared<FMockOpenXRTrackingSystem, ESPMode::ThreadSafe>();
				GEngine->XRSystem = Mock;

				const XrInstance I1 = MakeInstance(0x1001);
				const XrSession S1 = MakeSession(0x2001);
				Mock->Hmd.MockInstance = I1;
				Mock->Hmd.MockSession = S1;

				FInputHapticsExtensionPlugin Plugin;
				TestTrue(TEXT("instance == live I1"), Plugin.GetOpenXRInstance() == I1);
				TestTrue(TEXT("session == live S1"), Plugin.GetOpenXRSession() == S1);

				// Simulate a Link session recreation: the HMD now exposes a NEW
				// instance/session. A handle cached once (the bug) would still
				// return I1/S1; the fix fetches live, so it must return I2/S2.
				const XrInstance I2 = MakeInstance(0x1002);
				const XrSession S2 = MakeSession(0x2002);
				Mock->Hmd.MockInstance = I2;
				Mock->Hmd.MockSession = S2;
				TestTrue(TEXT("instance reflects recreation (live, not cached)"), Plugin.GetOpenXRInstance() == I2);
				TestTrue(TEXT("session reflects recreation (live, not cached)"), Plugin.GetOpenXRSession() == S2);

				// No OpenXR system -> getters return XR_NULL_HANDLE, so the
				// InstanceAndSessionAreValid guard fails gracefully instead of
				// handing a stale/dead handle to xrApplyHapticFeedback.
				GEngine->XRSystem.Reset();
				TestTrue(TEXT("instance null when no OpenXR system"), Plugin.GetOpenXRInstance() == XR_NULL_HANDLE);
				TestTrue(TEXT("session null when no OpenXR system"), Plugin.GetOpenXRSession() == XR_NULL_HANDLE);

				GEngine->XRSystem = PrevXRSystem;
			});
		});
	}
} // namespace OculusXRInput

#endif // WITH_DEV_AUTOMATION_TESTS
