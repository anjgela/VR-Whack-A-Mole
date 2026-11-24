// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IOculusXRInputModule.h"

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
#include "GenericPlatform/IInputInterface.h"
#include "IInputDevice.h"
#include "OculusXRInputState.h"

DEFINE_LOG_CATEGORY_STATIC(LogOcInput, Log, All);

#define OVR_HAP_LOGGING 0

class UHapticFeedbackEffect_Base;
struct FActiveHapticFeedbackEffect;
struct FOculusXRHapticsDesc;

namespace OculusXRInput
{

	class IOculusXRInputBase
	{
	public:
		virtual ~IOculusXRInputBase() = default;

		virtual void PlayHapticEffect(UHapticFeedbackEffect_Base* HapticEffect,
			EControllerHand Hand,
			EOculusXRHandHapticsLocation Location = EOculusXRHandHapticsLocation::Hand,
			bool bAppend = false,
			float Scale = 1.f,
			bool bLoop = false) = 0;
		virtual void PlayAmplitudeEnvelopeHapticEffect(EControllerHand Hand, int SamplesCount, void* Samples, int SampleRate = -1) = 0;
		virtual void SetHapticsByValue(float Frequency, float Amplitude, EControllerHand Hand, EOculusXRHandHapticsLocation Location = EOculusXRHandHapticsLocation::Hand) = 0;

		virtual float GetControllerSampleRateHz(EControllerHand Hand) const = 0;
		virtual int GetMaxHapticDuration(EControllerHand Hand) const = 0;
	};

	//-------------------------------------------------------------------------------------------------
	// FOculusXRInput
	//-------------------------------------------------------------------------------------------------

	class FOculusXRInput : public IInputDevice
	{
	public:
		/** Constructor that takes an initial message handler that will receive motion controller events */
		FOculusXRInput(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler);

		/** Clean everything up */
		virtual ~FOculusXRInput();

		static void PreInit();

		// IInputDevice overrides
		virtual void Tick(float DeltaTime) override;
		virtual void SendControllerEvents() override;
		virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;
		virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
		virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override;
		virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& Values) override;
		virtual bool SupportsForceFeedback(int32 ControllerId) override;

		void PlayHapticEffect(
			UHapticFeedbackEffect_Base* HapticEffect,
			EControllerHand Hand,
			EOculusXRHandHapticsLocation Location = EOculusXRHandHapticsLocation::Hand,
			bool bAppend = false,
			float Scale = 1.f,
			bool bLoop = false);
		void PlayAmplitudeEnvelopeHapticEffect(EControllerHand Hand, int SamplesCount, void* Samples, int SampleRate = -1);
		void SetHapticsByValue(float Frequency, float Amplitude, EControllerHand Hand, EOculusXRHandHapticsLocation Location = EOculusXRHandHapticsLocation::Hand);

		float GetControllerSampleRateHz(EControllerHand Hand);
		int GetMaxHapticDuration(EControllerHand Hand);

		static void ShutdownXRFunctionLibrary();

	private:
		/** The recipient of motion controller input events */
		TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;

	protected:
		static TSharedPtr<IOculusXRInputBase> GetOculusXRInputBaseImpl();
		static TSharedPtr<IOculusXRInputBase> FunctionLibraryImpl;
	};

} // namespace OculusXRInput

#endif // OCULUS_INPUT_SUPPORTED_PLATFORMS
