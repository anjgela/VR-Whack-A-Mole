// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IOculusXRInputModule.h"

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
#include "IMotionController.h"
#include "InputCoreTypes.h"
#include "OculusXRInputFunctionLibrary.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"

namespace OculusXRInput
{

	//-------------------------------------------------------------------------------------------------
	// Button names
	//-------------------------------------------------------------------------------------------------

	enum class EOculusHandButton
	{
		Thumb,
		Index,
		Middle,
		Ring,
		Pinky,
		System,
		Menu,
		TotalButtonCount
	};

	enum class EOculusHandAxes
	{
		Thumb,
		Index,
		Middle,
		Ring,
		Pinky,
		TotalAxisCount
	};

	enum class EOculusMicrogestures
	{
		NoGesture,
		SwipeLeft,
		SwipeRight,
		SwipeForward,
		SwipeBack,
		TapThumb,
		TotalGestureCount
	};

	//-------------------------------------------------------------------------------------------------
	// FOculusKey
	//-------------------------------------------------------------------------------------------------

	struct FOculusKey
	{
		static const FKey OculusTouch_Left_Thumbstick;
		static const FKey OculusTouch_Left_Trigger;
		static const FKey OculusTouch_Left_FaceButton1; // X or A
		static const FKey OculusTouch_Left_FaceButton2; // Y or B
		static const FKey OculusTouch_Left_IndexPointing;
		static const FKey OculusTouch_Left_Trigger_Proximity;
		static const FKey OculusTouch_Left_ThumbUp;
		static const FKey OculusTouch_Left_Thumb_Proximity;
		static const FKey OculusTouch_Left_ThumbRest;
		static const FKey OculusTouch_Left_ThumbRest_Force;
		static const FKey OculusTouch_Left_Stylus_Force;
		static const FKey OculusTouch_Left_IndexTrigger_Curl;
		static const FKey OculusTouch_Left_IndexTrigger_Slide;
		static const FKey OculusTouch_Left_IndexTrigger_Force;

		static const FKey OculusTouch_Right_Thumbstick;
		static const FKey OculusTouch_Right_Trigger;
		static const FKey OculusTouch_Right_FaceButton1; // X or A
		static const FKey OculusTouch_Right_FaceButton2; // Y or B
		static const FKey OculusTouch_Right_IndexPointing;
		static const FKey OculusTouch_Right_Trigger_Proximity;
		static const FKey OculusTouch_Right_ThumbUp;
		static const FKey OculusTouch_Right_Thumb_Proximity;
		static const FKey OculusTouch_Right_ThumbRest;
		static const FKey OculusTouch_Right_ThumbRest_Force;
		static const FKey OculusTouch_Right_Stylus_Force;
		static const FKey OculusTouch_Right_IndexTrigger_Curl;
		static const FKey OculusTouch_Right_IndexTrigger_Slide;
		static const FKey OculusTouch_Right_IndexTrigger_Force;

		static const FKey OculusRemote_DPad_Up;
		static const FKey OculusRemote_DPad_Down;
		static const FKey OculusRemote_DPad_Left;
		static const FKey OculusRemote_DPad_Right;

		static const FKey OculusRemote_Enter;
		static const FKey OculusRemote_Back;

		static const FKey OculusRemote_VolumeUp;
		static const FKey OculusRemote_VolumeDown;
		static const FKey OculusRemote_Home;

		static const FKey OculusHand_Left_ThumbPinch;
		static const FKey OculusHand_Left_IndexPinch;
		static const FKey OculusHand_Left_MiddlePinch;
		static const FKey OculusHand_Left_RingPinch;
		static const FKey OculusHand_Left_PinkyPinch;

		static const FKey OculusHand_Right_ThumbPinch;
		static const FKey OculusHand_Right_IndexPinch;
		static const FKey OculusHand_Right_MiddlePinch;
		static const FKey OculusHand_Right_RingPinch;
		static const FKey OculusHand_Right_PinkyPinch;

		static const FKey OculusHand_Left_SystemGesture;
		static const FKey OculusHand_Right_SystemGesture;

		static const FKey OculusHand_Left_ThumbPinchStrength;
		static const FKey OculusHand_Left_IndexPinchStrength;
		static const FKey OculusHand_Left_MiddlePinchStrength;
		static const FKey OculusHand_Left_RingPinchStrength;
		static const FKey OculusHand_Left_PinkyPinchStrength;

		static const FKey OculusHand_Right_ThumbPinchStrength;
		static const FKey OculusHand_Right_IndexPinchStrength;
		static const FKey OculusHand_Right_MiddlePinchStrength;
		static const FKey OculusHand_Right_RingPinchStrength;
		static const FKey OculusHand_Right_PinkyPinchStrength;

		static const FKey OculusHand_Left_MicrogestureSwipeLeft;
		static const FKey OculusHand_Left_MicrogestureSwipeRight;
		static const FKey OculusHand_Left_MicrogestureSwipeForward;
		static const FKey OculusHand_Left_MicrogestureSwipeBack;
		static const FKey OculusHand_Left_MicrogestureTapThumb;

		static const FKey OculusHand_Right_MicrogestureSwipeLeft;
		static const FKey OculusHand_Right_MicrogestureSwipeRight;
		static const FKey OculusHand_Right_MicrogestureSwipeForward;
		static const FKey OculusHand_Right_MicrogestureSwipeBack;
		static const FKey OculusHand_Right_MicrogestureTapThumb;
	};

	//-------------------------------------------------------------------------------------------------
	// FOculusKeyNames
	//-------------------------------------------------------------------------------------------------

	struct FOculusKeyNames
	{
		typedef FName Type;

		static const FName OculusTouch_Left_Thumbstick;
		static const FName OculusTouch_Left_Trigger;
		static const FName OculusTouch_Left_FaceButton1; // X or A
		static const FName OculusTouch_Left_FaceButton2; // Y or B
		static const FName OculusTouch_Left_IndexPointing;
		static const FName OculusTouch_Left_Trigger_Proximity;
		static const FName OculusTouch_Left_ThumbUp;
		static const FName OculusTouch_Left_Thumb_Proximity;
		static const FName OculusTouch_Left_ThumbRest;

		static const FName OculusTouch_Right_Thumbstick;
		static const FName OculusTouch_Right_Trigger;
		static const FName OculusTouch_Right_FaceButton1; // X or A
		static const FName OculusTouch_Right_FaceButton2; // Y or B
		static const FName OculusTouch_Right_IndexPointing;
		static const FName OculusTouch_Right_Trigger_Proximity;
		static const FName OculusTouch_Right_ThumbUp;
		static const FName OculusTouch_Right_Thumb_Proximity;
		static const FName OculusTouch_Right_ThumbRest;

		static const FName OculusRemote_DPad_Up;
		static const FName OculusRemote_DPad_Down;
		static const FName OculusRemote_DPad_Left;
		static const FName OculusRemote_DPad_Right;

		static const FName OculusRemote_Enter;
		static const FName OculusRemote_Back;

		static const FName OculusRemote_VolumeUp;
		static const FName OculusRemote_VolumeDown;
		static const FName OculusRemote_Home;

		static const FName OculusHand_Left_ThumbPinch;
		static const FName OculusHand_Left_IndexPinch;
		static const FName OculusHand_Left_MiddlePinch;
		static const FName OculusHand_Left_RingPinch;
		static const FName OculusHand_Left_PinkyPinch;

		static const FName OculusHand_Right_ThumbPinch;
		static const FName OculusHand_Right_IndexPinch;
		static const FName OculusHand_Right_MiddlePinch;
		static const FName OculusHand_Right_RingPinch;
		static const FName OculusHand_Right_PinkyPinch;

		static const FName OculusHand_Left_SystemGesture;
		static const FName OculusHand_Right_SystemGesture;

		static const FName OculusHand_Left_ThumbPinchStrength;
		static const FName OculusHand_Left_IndexPinchStrength;
		static const FName OculusHand_Left_MiddlePinchStrength;
		static const FName OculusHand_Left_RingPinchStrength;
		static const FName OculusHand_Left_PinkyPinchStrength;

		static const FName OculusHand_Right_ThumbPinchStrength;
		static const FName OculusHand_Right_IndexPinchStrength;
		static const FName OculusHand_Right_MiddlePinchStrength;
		static const FName OculusHand_Right_RingPinchStrength;
		static const FName OculusHand_Right_PinkyPinchStrength;
	};

	//-------------------------------------------------------------------------------------------------
	// FOculusButtonState -  Digital button state
	//-------------------------------------------------------------------------------------------------

	struct FOculusButtonState
	{
		/** The Unreal button this maps to.  Different depending on whether this is the Left or Right hand controller */
		FName Key;

		/** The Unreal button this maps to.  Different depending on whether this is the Left or Right hand controller */
		FName EmulatedKey;

		/** Whether we're pressed or not.  While pressed, we will generate repeat presses on a timer */
		bool bIsPressed;

		/** Next time a repeat event should be generated for each button */
		double NextRepeatTime;

		/** Default constructor that just sets sensible defaults */
		FOculusButtonState()
			: Key(NAME_None), EmulatedKey(NAME_None), bIsPressed(false), NextRepeatTime(0.0)
		{
		}
	};

	//-------------------------------------------------------------------------------------------------
	// FOculusTouchCapacitiveState - Capacitive Axis State
	//-------------------------------------------------------------------------------------------------

	struct FOculusAxisState
	{
		/** The axis that this button state maps to */
		FName Axis;

		/** How close the finger is to this button, from 0.f to 1.f */
		float State;

		FOculusAxisState()
			: Axis(NAME_None)
			, State(0.f)
		{
		}
	};

	struct FOculusMicrogestureState
	{
		/** The Unreal button this maps to.  Different depending on whether this is the Left or Right hand controller */
		FName Key;

		/** Wether the microgesture was detected this frame */
		bool bIsActive;

		FOculusMicrogestureState()
			: Key(NAME_None), bIsActive(false)
		{
		}
	};

	struct FOculusControllerState
	{
		/** True if the device is connected, otherwise false */
		bool bIsConnected;

		/** True if position is being tracked, otherwise false */
		bool bIsPositionTracked;

		/** True if position is valid (tracked or estimated), otherwise false */
		bool bIsPositionValid;

		/** True if orientation is being tracked, otherwise false */
		bool bIsOrientationTracked;

		/** True if orientation is valid (tracked or estimated), otherwise false */
		bool bIsOrientationValid;

		FOculusControllerState()
			: bIsConnected(false), bIsPositionTracked(false), bIsPositionValid(false), bIsOrientationTracked(false), bIsOrientationValid(false)
		{
		}
	};

	//-------------------------------------------------------------------------------------------------
	// FOculusHandControllerState - Input state for an Oculus Hands
	//-------------------------------------------------------------------------------------------------

	struct FOculusHandControllerState : FOculusControllerState
	{
		/** True if the pointer pose for hands is valid */
		bool bIsPointerPoseValid;

		/** True if the current hand is the dominant hand */
		bool bIsDominantHand;

		/** Scale of the hand */
		float HandScale;

		/** Pose of the pointer */
		FTransform PointerPose;

		/** Tracking confidence of hand tracking */
		EOculusXRTrackingConfidence TrackingConfidence;

		/** Finger Pinch States **/
		FOculusButtonState HandButtons[(int32)EOculusHandButton::TotalButtonCount];

		/** Finger Pinch Strength States **/
		FOculusAxisState HandAxes[(int32)EOculusHandAxes::TotalAxisCount];

		/** Finger Pinch Strength States **/
		FOculusMicrogestureState Microgestures[(int32)EOculusMicrogestures::TotalGestureCount];

		/** Finger Confidences **/
		EOculusXRTrackingConfidence FingerConfidences[(int32)EOculusHandAxes::TotalAxisCount] = {};

		FQuat BoneRotations[(int32)EOculusXRBone::Bone_Max];

		FOculusHandControllerState(const EControllerHand Hand)
		{
			TrackingConfidence = EOculusXRTrackingConfidence::Low;
			bIsPointerPoseValid = false;
			bIsDominantHand = false;
			HandScale = 0.0f;
			PointerPose = FTransform::Identity;

			for (FOculusButtonState& Button : HandButtons)
			{
				Button.bIsPressed = false;
				Button.NextRepeatTime = 0.0;
			}

			for (int32 i = 0; i < (int32)EOculusXRBone::Bone_Max; i++)
			{
				BoneRotations[i] = FQuat::Identity;
			}

			HandButtons[(int32)EOculusHandButton::Thumb].Key = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_ThumbPinch : FOculusKeyNames::OculusHand_Right_ThumbPinch;
			HandButtons[(int32)EOculusHandButton::Index].Key = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_IndexPinch : FOculusKeyNames::OculusHand_Right_IndexPinch;
			HandButtons[(int32)EOculusHandButton::Middle].Key = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_MiddlePinch : FOculusKeyNames::OculusHand_Right_MiddlePinch;
			HandButtons[(int32)EOculusHandButton::Ring].Key = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_RingPinch : FOculusKeyNames::OculusHand_Right_RingPinch;
			HandButtons[(int32)EOculusHandButton::Pinky].Key = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_PinkyPinch : FOculusKeyNames::OculusHand_Right_PinkyPinch;
			HandButtons[(int32)EOculusHandButton::System].Key = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_SystemGesture : FOculusKeyNames::OculusHand_Right_SystemGesture;
			HandButtons[(int32)EOculusHandButton::Menu].Key = (Hand == EControllerHand::Left) ? FGamepadKeyNames::SpecialLeft : FGamepadKeyNames::SpecialRight;

			HandAxes[(int32)EOculusHandAxes::Thumb].Axis = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_ThumbPinchStrength : FOculusKeyNames::OculusHand_Right_ThumbPinchStrength;
			HandAxes[(int32)EOculusHandAxes::Index].Axis = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_IndexPinchStrength : FOculusKeyNames::OculusHand_Right_IndexPinchStrength;
			HandAxes[(int32)EOculusHandAxes::Middle].Axis = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_MiddlePinchStrength : FOculusKeyNames::OculusHand_Right_MiddlePinchStrength;
			HandAxes[(int32)EOculusHandAxes::Ring].Axis = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_RingPinchStrength : FOculusKeyNames::OculusHand_Right_RingPinchStrength;
			HandAxes[(int32)EOculusHandAxes::Pinky].Axis = (Hand == EControllerHand::Left) ? FOculusKeyNames::OculusHand_Left_PinkyPinchStrength : FOculusKeyNames::OculusHand_Right_PinkyPinchStrength;

			Microgestures[(int32)EOculusMicrogestures::SwipeLeft].Key = (Hand == EControllerHand::Left) ? FOculusKey::OculusHand_Left_MicrogestureSwipeLeft.GetFName() : FOculusKey::OculusHand_Right_MicrogestureSwipeLeft.GetFName();
			Microgestures[(int32)EOculusMicrogestures::SwipeRight].Key = (Hand == EControllerHand::Left) ? FOculusKey::OculusHand_Left_MicrogestureSwipeRight.GetFName() : FOculusKey::OculusHand_Right_MicrogestureSwipeRight.GetFName();
			Microgestures[(int32)EOculusMicrogestures::SwipeForward].Key = (Hand == EControllerHand::Left) ? FOculusKey::OculusHand_Left_MicrogestureSwipeForward.GetFName() : FOculusKey::OculusHand_Right_MicrogestureSwipeForward.GetFName();
			Microgestures[(int32)EOculusMicrogestures::SwipeBack].Key = (Hand == EControllerHand::Left) ? FOculusKey::OculusHand_Left_MicrogestureSwipeBack.GetFName() : FOculusKey::OculusHand_Right_MicrogestureSwipeBack.GetFName();
			Microgestures[(int32)EOculusMicrogestures::TapThumb].Key = (Hand == EControllerHand::Left) ? FOculusKey::OculusHand_Left_MicrogestureTapThumb.GetFName() : FOculusKey::OculusHand_Right_MicrogestureTapThumb.GetFName();
		}

		FOculusHandControllerState()
		{
		}
	};

	const float DefaultInitialButtonRepeatDelay = 0.2f;
	const float DefaultButtonRepeatDelay = 0.1f;

} // namespace OculusXRInput

#endif // OCULUS_INPUT_SUPPORTED_PLATFORMS
