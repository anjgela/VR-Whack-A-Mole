// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRInput.h"
#include "OculusXRInputOpenXR.h"

#if OCULUS_INPUT_SUPPORTED_PLATFORMS

#define LOCTEXT_NAMESPACE "OculusXRInput"

namespace OculusXRInput
{

	const FKey FOculusKey::OculusTouch_Left_Thumbstick("OculusTouch_Left_Thumbstick");
	const FKey FOculusKey::OculusTouch_Left_Trigger("OculusTouch_Left_Trigger");
	const FKey FOculusKey::OculusTouch_Left_FaceButton1("OculusTouch_Left_FaceButton1");
	const FKey FOculusKey::OculusTouch_Left_FaceButton2("OculusTouch_Left_FaceButton2");
	const FKey FOculusKey::OculusTouch_Left_IndexPointing("OculusTouch_Left_IndexPointing");
	const FKey FOculusKey::OculusTouch_Left_Trigger_Proximity("OculusTouch_Left_Trigger_Proximity");
	const FKey FOculusKey::OculusTouch_Left_ThumbUp("OculusTouch_Left_ThumbUp");
	const FKey FOculusKey::OculusTouch_Left_Thumb_Proximity("OculusTouch_Left_Thumb_Proximity");
	const FKey FOculusKey::OculusTouch_Left_ThumbRest("OculusTouch_Left_ThumbRest");

	const FKey FOculusKey::OculusTouch_Left_ThumbRest_Force("OculusTouch_Left_ThumbRest_Force");
	const FKey FOculusKey::OculusTouch_Left_Stylus_Force("OculusTouch_Left_Stylus_Force");
	const FKey FOculusKey::OculusTouch_Left_IndexTrigger_Curl("OculusTouch_Left_IndexTrigger_Curl");
	const FKey FOculusKey::OculusTouch_Left_IndexTrigger_Slide("OculusTouch_Left_IndexTrigger_Slide");
	const FKey FOculusKey::OculusTouch_Left_IndexTrigger_Force("OculusTouch_Left_IndexTrigger_Force");

	const FKey FOculusKey::OculusTouch_Right_Thumbstick("OculusTouch_Right_Thumbstick");
	const FKey FOculusKey::OculusTouch_Right_Trigger("OculusTouch_Right_Trigger");
	const FKey FOculusKey::OculusTouch_Right_FaceButton1("OculusTouch_Right_FaceButton1");
	const FKey FOculusKey::OculusTouch_Right_FaceButton2("OculusTouch_Right_FaceButton2");
	const FKey FOculusKey::OculusTouch_Right_IndexPointing("OculusTouch_Right_IndexPointing");
	const FKey FOculusKey::OculusTouch_Right_Trigger_Proximity("OculusTouch_Right_Trigger_Proximity");
	const FKey FOculusKey::OculusTouch_Right_ThumbUp("OculusTouch_Right_ThumbUp");
	const FKey FOculusKey::OculusTouch_Right_Thumb_Proximity("OculusTouch_Right_Thumb_Proximity");
	const FKey FOculusKey::OculusTouch_Right_ThumbRest("OculusTouch_Right_ThumbRest");

	const FKey FOculusKey::OculusTouch_Right_ThumbRest_Force("OculusTouch_Right_ThumbRest_Force");
	const FKey FOculusKey::OculusTouch_Right_Stylus_Force("OculusTouch_Right_Stylus_Force");
	const FKey FOculusKey::OculusTouch_Right_IndexTrigger_Curl("OculusTouch_Right_IndexTrigger_Curl");
	const FKey FOculusKey::OculusTouch_Right_IndexTrigger_Slide("OculusTouch_Right_IndexTrigger_Slide");
	const FKey FOculusKey::OculusTouch_Right_IndexTrigger_Force("OculusTouch_Right_IndexTrigger_Force");

	const FKey FOculusKey::OculusRemote_DPad_Down("OculusRemote_DPad_Down");
	const FKey FOculusKey::OculusRemote_DPad_Up("OculusRemote_DPad_Up");
	const FKey FOculusKey::OculusRemote_DPad_Left("OculusRemote_DPad_Left");
	const FKey FOculusKey::OculusRemote_DPad_Right("OculusRemote_DPad_Right");
	const FKey FOculusKey::OculusRemote_Enter("OculusRemote_Enter");
	const FKey FOculusKey::OculusRemote_Back("OculusRemote_Back");
	const FKey FOculusKey::OculusRemote_VolumeUp("OculusRemote_VolumeUp");
	const FKey FOculusKey::OculusRemote_VolumeDown("OculusRemote_VolumeDown");
	const FKey FOculusKey::OculusRemote_Home("OculusRemote_Home");

	const FKey FOculusKey::OculusHand_Left_ThumbPinch("OculusHand_Left_ThumbPinch");
	const FKey FOculusKey::OculusHand_Left_IndexPinch("OculusHand_Left_IndexPinch");
	const FKey FOculusKey::OculusHand_Left_MiddlePinch("OculusHand_Left_MiddlePinch");
	const FKey FOculusKey::OculusHand_Left_RingPinch("OculusHand_Left_RingPinch");
	const FKey FOculusKey::OculusHand_Left_PinkyPinch("OculusHand_Left_PinkPinch");

	const FKey FOculusKey::OculusHand_Right_ThumbPinch("OculusHand_Right_ThumbPinch");
	const FKey FOculusKey::OculusHand_Right_IndexPinch("OculusHand_Right_IndexPinch");
	const FKey FOculusKey::OculusHand_Right_MiddlePinch("OculusHand_Right_MiddlePinch");
	const FKey FOculusKey::OculusHand_Right_RingPinch("OculusHand_Right_RingPinch");
	const FKey FOculusKey::OculusHand_Right_PinkyPinch("OculusHand_Right_PinkPinch");

	const FKey FOculusKey::OculusHand_Left_SystemGesture("OculusHand_Left_SystemGesture");
	const FKey FOculusKey::OculusHand_Right_SystemGesture("OculusHand_Right_SystemGesture");

	const FKey FOculusKey::OculusHand_Left_ThumbPinchStrength("OculusHand_Left_ThumbPinchStrength");
	const FKey FOculusKey::OculusHand_Left_IndexPinchStrength("OculusHand_Left_IndexPinchStrength");
	const FKey FOculusKey::OculusHand_Left_MiddlePinchStrength("OculusHand_Left_MiddlePinchStrength");
	const FKey FOculusKey::OculusHand_Left_RingPinchStrength("OculusHand_Left_RingPinchStrength");
	const FKey FOculusKey::OculusHand_Left_PinkyPinchStrength("OculusHand_Left_PinkPinchStrength");

	const FKey FOculusKey::OculusHand_Right_ThumbPinchStrength("OculusHand_Right_ThumbPinchStrength");
	const FKey FOculusKey::OculusHand_Right_IndexPinchStrength("OculusHand_Right_IndexPinchStrength");
	const FKey FOculusKey::OculusHand_Right_MiddlePinchStrength("OculusHand_Right_MiddlePinchStrength");
	const FKey FOculusKey::OculusHand_Right_RingPinchStrength("OculusHand_Right_RingPinchStrength");
	const FKey FOculusKey::OculusHand_Right_PinkyPinchStrength("OculusHand_Right_PinkPinchStrength");

	const FKey FOculusKey::OculusHand_Left_MicrogestureSwipeLeft("OculusHand_Left_MicrogestureSwipeLeft");
	const FKey FOculusKey::OculusHand_Left_MicrogestureSwipeRight("OculusHand_Left_MicrogestureSwipeRight");
	const FKey FOculusKey::OculusHand_Left_MicrogestureSwipeForward("OculusHand_Left_MicrogestureSwipeForward");
	const FKey FOculusKey::OculusHand_Left_MicrogestureSwipeBack("OculusHand_Left_MicrogestureSwipeBack");
	const FKey FOculusKey::OculusHand_Left_MicrogestureTapThumb("OculusHand_Left_MicrogestureTapThumb");

	const FKey FOculusKey::OculusHand_Right_MicrogestureSwipeLeft("OculusHand_Right_MicrogestureSwipeLeft");
	const FKey FOculusKey::OculusHand_Right_MicrogestureSwipeRight("OculusHand_Right_MicrogestureSwipeRight");
	const FKey FOculusKey::OculusHand_Right_MicrogestureSwipeForward("OculusHand_Right_MicrogestureSwipeForward");
	const FKey FOculusKey::OculusHand_Right_MicrogestureSwipeBack("OculusHand_Right_MicrogestureSwipeBack");
	const FKey FOculusKey::OculusHand_Right_MicrogestureTapThumb("OculusHand_Right_MicrogestureTapThumb");

	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_Thumbstick("OculusTouch_Left_Thumbstick");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_Trigger("OculusTouch_Left_Trigger");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_FaceButton1("OculusTouch_Left_FaceButton1");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_FaceButton2("OculusTouch_Left_FaceButton2");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_IndexPointing("OculusTouch_Left_IndexPointing");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_Trigger_Proximity("OculusTouch_Left_Trigger_Proximity");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_ThumbUp("OculusTouch_Left_ThumbUp");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_Thumb_Proximity("OculusTouch_Left_Thumb_Proximity");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Left_ThumbRest("OculusTouch_Left_ThumbRest");

	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_Thumbstick("OculusTouch_Right_Thumbstick");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_Trigger("OculusTouch_Right_Trigger");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_FaceButton1("OculusTouch_Right_FaceButton1");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_FaceButton2("OculusTouch_Right_FaceButton2");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_IndexPointing("OculusTouch_Right_IndexPointing");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_Trigger_Proximity("OculusTouch_Right_Trigger_Proximity");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_ThumbUp("OculusTouch_Right_ThumbUp");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_Thumb_Proximity("OculusTouch_Right_Thumb_Proximity");
	const FOculusKeyNames::Type FOculusKeyNames::OculusTouch_Right_ThumbRest("OculusTouch_Right_ThumbRest");

	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_DPad_Down("OculusRemote_DPad_Down");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_DPad_Up("OculusRemote_DPad_Up");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_DPad_Left("OculusRemote_DPad_Left");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_DPad_Right("OculusRemote_DPad_Right");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_Enter("OculusRemote_Enter");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_Back("OculusRemote_Back");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_VolumeUp("OculusRemote_VolumeUp");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_VolumeDown("OculusRemote_VolumeDown");
	const FOculusKeyNames::Type FOculusKeyNames::OculusRemote_Home("OculusRemote_Home");

	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_ThumbPinch("OculusHand_Left_ThumbPinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_IndexPinch("OculusHand_Left_IndexPinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_MiddlePinch("OculusHand_Left_MiddlePinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_RingPinch("OculusHand_Left_RingPinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_PinkyPinch("OculusHand_Left_PinkPinch");

	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_ThumbPinch("OculusHand_Right_ThumbPinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_IndexPinch("OculusHand_Right_IndexPinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_MiddlePinch("OculusHand_Right_MiddlePinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_RingPinch("OculusHand_Right_RingPinch");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_PinkyPinch("OculusHand_Right_PinkPinch");

	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_SystemGesture("OculusHand_Left_SystemGesture");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_SystemGesture("OculusHand_Right_SystemGesture");

	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_ThumbPinchStrength("OculusHand_Left_ThumbPinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_IndexPinchStrength("OculusHand_Left_IndexPinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_MiddlePinchStrength("OculusHand_Left_MiddlePinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_RingPinchStrength("OculusHand_Left_RingPinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Left_PinkyPinchStrength("OculusHand_Left_PinkPinchStrength");

	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_ThumbPinchStrength("OculusHand_Right_ThumbPinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_IndexPinchStrength("OculusHand_Right_IndexPinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_MiddlePinchStrength("OculusHand_Right_MiddlePinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_RingPinchStrength("OculusHand_Right_RingPinchStrength");
	const FOculusKeyNames::Type FOculusKeyNames::OculusHand_Right_PinkyPinchStrength("OculusHand_Right_PinkPinchStrength");

	FOculusXRInput::FOculusXRInput(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
		: MessageHandler(InMessageHandler)
	{
		UE_LOG(LogOcInput, Log, TEXT("OculusXRInput is initialized"));
	}

	FOculusXRInput::~FOculusXRInput()
	{
	}

	void FOculusXRInput::PreInit()
	{
		// Register the FKeys
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_Thumbstick, LOCTEXT("OculusTouch_Left_Thumbstick", "Oculus Touch (L) Thumbstick CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_FaceButton1, LOCTEXT("OculusTouch_Left_FaceButton1", "Oculus Touch (L) X Button CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_Trigger, LOCTEXT("OculusTouch_Left_Trigger", "Oculus Touch (L) Trigger CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_FaceButton2, LOCTEXT("OculusTouch_Left_FaceButton2", "Oculus Touch (L) Y Button CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_IndexPointing, LOCTEXT("OculusTouch_Left_IndexPointing", "Oculus Touch (L) Pointing CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_Trigger_Proximity, LOCTEXT("OculusTouch_Left_Trigger_Proximity", "Oculus Touch (L) Trigger Proximity CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_ThumbUp, LOCTEXT("OculusTouch_Left_ThumbUp", "Oculus Touch (L) Thumb Up CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_Thumb_Proximity, LOCTEXT("OculusTouch_Left_Thumb_Proximity", "Oculus Touch (L) Thumbstick Proximity CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_ThumbRest, LOCTEXT("OculusTouch_Left_ThumbRest", "Oculus Touch (L) Thumb Rest CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_ThumbRest_Force, LOCTEXT("OculusTouch_Left_ThumbRest_Force", "Oculus Touch (L) Thumb Rest Force"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_Stylus_Force, LOCTEXT("OculusTouch_Left_Stylus_Force", "Oculus Touch (L) Stylus Force"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_IndexTrigger_Curl, LOCTEXT("OculusTouch_Left_IndexTrigger_Curl", "Oculus Touch (L) Trigger Curl CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_IndexTrigger_Slide, LOCTEXT("OculusTouch_Left_IndexTrigger_Slide", "Oculus Touch (L) Trigger Slide CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Left_IndexTrigger_Force, LOCTEXT("OculusTouch_Left_IndexTrigger_Force", "Oculus Touch (L) Trigger Force"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_Thumbstick, LOCTEXT("OculusTouch_Right_Thumbstick", "Oculus Touch (R) Thumbstick CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_FaceButton1, LOCTEXT("OculusTouch_Right_FaceButton1", "Oculus Touch (R) A Button CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_Trigger, LOCTEXT("OculusTouch_Right_Trigger", "Oculus Touch (R) Trigger CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_FaceButton2, LOCTEXT("OculusTouch_Right_FaceButton2", "Oculus Touch (R) B Button CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_IndexPointing, LOCTEXT("OculusTouch_Right_IndexPointing", "Oculus Touch (R) Pointing CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_Trigger_Proximity, LOCTEXT("OculusTouch_Right_Trigger_Proximity", "Oculus Touch (R) Trigger Proximity CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_ThumbUp, LOCTEXT("OculusTouch_Right_ThumbUp", "Oculus Touch (R) Thumb Up CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_Thumb_Proximity, LOCTEXT("OculusTouch_Right_Thumb_Proximity", "Oculus Touch (R) Thumbstick Proximity CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_ThumbRest, LOCTEXT("OculusTouch_Right_ThumbRest", "Oculus Touch (R) Thumb Rest CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_ThumbRest_Force, LOCTEXT("OculusTouch_Right_ThumbRest_Force", "Oculus Touch (R) Thumb Rest Force"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_Stylus_Force, LOCTEXT("OculusTouch_Right_Stylus_Force", "Oculus Touch (R) Stylus Force"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_IndexTrigger_Curl, LOCTEXT("OculusTouch_Right_IndexTrigger_Curl", "Oculus Touch (R) Trigger Curl CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_IndexTrigger_Slide, LOCTEXT("OculusTouch_Right_IndexTrigger_Slide", "Oculus Touch (R) Trigger Slide CapTouch"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusTouch_Right_IndexTrigger_Force, LOCTEXT("OculusTouch_Right_IndexTrigger_Force", "Oculus Touch (R) Trigger Force"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "OculusTouch"));

		EKeys::AddMenuCategoryDisplayInfo("OculusRemote", LOCTEXT("OculusRemoteSubCategory", "Oculus Remote"), TEXT("GraphEditor.PadEvent_16x"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_DPad_Up, LOCTEXT("OculusRemote_DPad_Up", "Oculus Remote D-pad Up"), FKeyDetails::GamepadKey, "OculusRemote"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_DPad_Down, LOCTEXT("OculusRemote_DPad_Down", "Oculus Remote D-pad Down"), FKeyDetails::GamepadKey, "OculusRemote"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_DPad_Left, LOCTEXT("OculusRemote_DPad_Left", "Oculus Remote D-pad Left"), FKeyDetails::GamepadKey, "OculusRemote"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_DPad_Right, LOCTEXT("OculusRemote_DPad_Right", "Oculus Remote D-pad Right"), FKeyDetails::GamepadKey, "OculusRemote"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_Enter, LOCTEXT("OculusRemote_Enter", "Oculus Remote Enter"), FKeyDetails::GamepadKey, "OculusRemote"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_Back, LOCTEXT("OculusRemote_Back", "Oculus Remote Back"), FKeyDetails::GamepadKey, "OculusRemote"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_VolumeUp, LOCTEXT("OculusRemote_VolumeUp", "Oculus Remote Volume Up"), FKeyDetails::GamepadKey, "OculusRemote"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_VolumeDown, LOCTEXT("OculusRemote_VolumeDown", "Oculus Remote Volume Down"), FKeyDetails::GamepadKey, "OculusRemote"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusRemote_Home, LOCTEXT("OculusRemote_Home", "Oculus Remote Home"), FKeyDetails::GamepadKey, "OculusRemote"));

		EKeys::AddMenuCategoryDisplayInfo("OculusHand", LOCTEXT("OculusHandSubCategory", "Oculus Hand"), TEXT("GraphEditor.PadEvent_16x"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_ThumbPinch, LOCTEXT("OculusHand_Left_ThumbPinch", "Oculus Hand (L) Thumb Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_IndexPinch, LOCTEXT("OculusHand_Left_IndexPinch", "Oculus Hand (L) Index Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MiddlePinch, LOCTEXT("OculusHand_Left_MiddlePinch", "Oculus Hand (L) Middle Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_RingPinch, LOCTEXT("OculusHand_Left_RingPinch", "Oculus Hand (L) Ring Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_PinkyPinch, LOCTEXT("OculusHand_Left_PinkyPinch", "Oculus Hand (L) Pinky Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_ThumbPinch, LOCTEXT("OculusHand_Right_ThumbPinch", "Oculus Hand (R) Thumb Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_IndexPinch, LOCTEXT("OculusHand_Right_IndexPinch", "Oculus Hand (R) Index Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MiddlePinch, LOCTEXT("OculusHand_Right_MiddlePinch", "Oculus Hand (R) Middle Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_RingPinch, LOCTEXT("OculusHand_Right_RingPinch", "Oculus Hand (R) Ring Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_PinkyPinch, LOCTEXT("OculusHand_Right_PinkyPinch", "Oculus Hand (R) Pinky Pinch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_SystemGesture, LOCTEXT("OculusHand_Left_SystemGesture", "Oculus Hand (L) System Gesture"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_SystemGesture, LOCTEXT("OculusHand_Right_SystemGesture", "Oculus Hand (R) System Gesture"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "OculusHand"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_ThumbPinchStrength, LOCTEXT("OculusHand_Left_ThumbPinchStrength", "Oculus Hand (L) Thumb Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_IndexPinchStrength, LOCTEXT("OculusHand_Left_IndexPinchStrength", "Oculus Hand (L) Index Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MiddlePinchStrength, LOCTEXT("OculusHand_Left_MiddlePinchStrength", "Oculus Hand (L) Middle Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_RingPinchStrength, LOCTEXT("OculusHand_Left_RingPinchStrength", "Oculus Hand (L) Ring Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_PinkyPinchStrength, LOCTEXT("OculusHand_Left_PinkyPinchStrength", "Oculus Hand (L) Pinky Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_ThumbPinchStrength, LOCTEXT("OculusHand_Right_ThumbPinchStrength", "Oculus Hand (R) Thumb Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_IndexPinchStrength, LOCTEXT("OculusHand_Right_IndexPinchStrength", "Oculus Hand (R) Index Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MiddlePinchStrength, LOCTEXT("OculusHand_Right_MiddlePinchStrength", "Oculus Hand (R) Middle Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_RingPinchStrength, LOCTEXT("OculusHand_Right_RingPinchStrength", "Oculus Hand (R) Ring Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_PinkyPinchStrength, LOCTEXT("OculusHand_Right_PinkyPinchStrength", "Oculus Hand (R) Pinky Pinch Strength"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, "OculusHand"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MicrogestureSwipeLeft, LOCTEXT("OculusHand_Left_MicrogestureSwipeLeft", "Oculus Hand (L) Microgesture - Swipe Left"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MicrogestureSwipeRight, LOCTEXT("OculusHand_Left_MicrogestureSwipeRight", "Oculus Hand (L) Microgesture - Swipe Right"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MicrogestureSwipeForward, LOCTEXT("OculusHand_Left_MicrogestureSwipeForward", "Oculus Hand (L) Microgesture - Swipe Forward"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MicrogestureSwipeBack, LOCTEXT("OculusHand_Left_MicrogestureSwipeBack", "Oculus Hand (L) Microgesture - Swipe Back"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Left_MicrogestureTapThumb, LOCTEXT("OculusHand_Left_MicrogestureTapThumb", "Oculus Hand (L) Microgesture - Tap Thumb"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));

		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MicrogestureSwipeLeft, LOCTEXT("OculusHand_Right_MicrogestureSwipeLeft", "Oculus Hand (R) Microgesture - Swipe Left"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MicrogestureSwipeRight, LOCTEXT("OculusHand_Right_MicrogestureSwipeRight", "Oculus Hand (R) Microgesture - Swipe Right"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MicrogestureSwipeForward, LOCTEXT("OculusHand_Right_MicrogestureSwipeForward", "Oculus Hand (R) Microgesture - Swipe Forward"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MicrogestureSwipeBack, LOCTEXT("OculusHand_Right_MicrogestureSwipeBack", "Oculus Hand (R) Microgesture - Swipe Back"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));
		EKeys::AddKey(FKeyDetails(FOculusKey::OculusHand_Right_MicrogestureTapThumb, LOCTEXT("OculusHand_Right_MicrogestureTapThumb", "Oculus Hand (R) Microgesture - Tap Thumb"), FKeyDetails::GamepadKey | FKeyDetails::Gesture, "OculusHand"));

		UE_LOG(LogOcInput, Log, TEXT("OculusXRInput pre-init called"));
	}

	void FOculusXRInput::Tick(float DeltaTime)
	{
	}

	void FOculusXRInput::SendControllerEvents()
	{
	}

	void FOculusXRInput::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
	{
		MessageHandler = InMessageHandler;
	}

	bool FOculusXRInput::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
	{
		// No exec commands supported, for now.
		return false;
	}

	void FOculusXRInput::SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
	{
	}

	void FOculusXRInput::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& Values)
	{
	}

	bool FOculusXRInput::SupportsForceFeedback(int32 ControllerId)
	{
		// Epic's OpenXR input device drives force feedback for Touch controllers; IInputDevice
		// defaults this to true, so it has to be answered explicitly.
		return false;
	}

	void FOculusXRInput::ShutdownXRFunctionLibrary()
	{
		FunctionLibraryImpl = nullptr;
	}

	void FOculusXRInput::PlayHapticEffect(
		UHapticFeedbackEffect_Base* HapticEffect,
		EControllerHand Hand,
		EOculusXRHandHapticsLocation Location,
		bool bAppend,
		float Scale,
		bool bLoop)
	{
		TSharedPtr<IOculusXRInputBase> Impl = GetOculusXRInputBaseImpl();
		return Impl->PlayHapticEffect(HapticEffect, Hand, Location, bAppend, Scale, bLoop);
	}

	void FOculusXRInput::PlayAmplitudeEnvelopeHapticEffect(EControllerHand Hand, int SamplesCount, void* Samples, int InSampleRate)
	{
		TSharedPtr<IOculusXRInputBase> Impl = GetOculusXRInputBaseImpl();
		Impl->PlayAmplitudeEnvelopeHapticEffect(Hand, SamplesCount, Samples, InSampleRate);
	}

	void FOculusXRInput::SetHapticsByValue(float Frequency, float Amplitude, EControllerHand Hand, EOculusXRHandHapticsLocation Location)
	{
		TSharedPtr<IOculusXRInputBase> Impl = GetOculusXRInputBaseImpl();
		return Impl->SetHapticsByValue(Frequency, Amplitude, Hand, Location);
	}

	TSharedPtr<IOculusXRInputBase> FOculusXRInput::FunctionLibraryImpl = nullptr;

	TSharedPtr<IOculusXRInputBase> FOculusXRInput::GetOculusXRInputBaseImpl()
	{
		if (FunctionLibraryImpl == nullptr)
		{
			FunctionLibraryImpl = MakeShared<FOculusXRInputOpenXR>();
		}
		return FunctionLibraryImpl;
	}

	float FOculusXRInput::GetControllerSampleRateHz(EControllerHand Hand)
	{
		TSharedPtr<IOculusXRInputBase> Impl = GetOculusXRInputBaseImpl();
		return Impl->GetControllerSampleRateHz(Hand);
	}

	int FOculusXRInput::GetMaxHapticDuration(EControllerHand Hand)
	{
		TSharedPtr<IOculusXRInputBase> Impl = GetOculusXRInputBaseImpl();
		return Impl->GetMaxHapticDuration(Hand);
	}
} // namespace OculusXRInput

#undef LOCTEXT_NAMESPACE
#endif // OCULUS_INPUT_SUPPORTED_PLATFORMS
