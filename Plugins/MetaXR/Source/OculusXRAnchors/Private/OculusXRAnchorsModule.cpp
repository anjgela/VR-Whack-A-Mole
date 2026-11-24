// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRAnchorsModule.h"

#if OCULUS_ANCHORS_SUPPORTED_PLATFORMS
#include "OculusXRHMDModule.h"
#include "OculusXRAnchors.h"
#include "OculusXRAnchorComponents.h"

DEFINE_LOG_CATEGORY(LogOculusXRAnchors);

#define LOCTEXT_NAMESPACE "OculusXRAnchors"

//-------------------------------------------------------------------------------------------------
// FOculusXRAnchorsModule
//-------------------------------------------------------------------------------------------------
void FOculusXRAnchorsModule::StartupModule()
{
	AnchorsXR = MakeShareable(new XRAnchors::FAnchorsXR());
	AnchorsXR->RegisterAsOpenXRExtension();

	AddCreateAnchorComponentInterface(this);
	Anchors.Initialize();
}

void FOculusXRAnchorsModule::ShutdownModule()
{
	Anchors.Teardown();
}

void FOculusXRAnchorsModule::AddCreateAnchorComponentInterface(IOculusXRCreateAnchorComponent* CastInterface)
{
	CreateComponentInterfaces.AddUnique(CastInterface);
}

void FOculusXRAnchorsModule::RemoveCreateAnchorComponentInterface(IOculusXRCreateAnchorComponent* CastInterface)
{
	CreateComponentInterfaces.Remove(CastInterface);
}

UOculusXRBaseAnchorComponent* FOculusXRAnchorsModule::CreateAnchorComponent(uint64 AnchorHandle, EOculusXRSpaceComponentType Type, UObject* Outer)
{
	for (auto& it : CreateComponentInterfaces)
	{
		auto comp = it->TryCreateAnchorComponent(AnchorHandle, Type, Outer);
		if (IsValid(comp))
		{
			return comp;
		}
	}

	return nullptr;
}

UOculusXRBaseAnchorComponent* FOculusXRAnchorsModule::TryCreateAnchorComponent(uint64 AnchorHandle, EOculusXRSpaceComponentType Type, UObject* Outer)
{
	switch (Type)
	{
		case EOculusXRSpaceComponentType::Locatable:
			return UOculusXRBaseAnchorComponent::FromSpace<UOculusXRLocatableAnchorComponent>(AnchorHandle, Outer);
		case EOculusXRSpaceComponentType::SpaceContainer:
			return UOculusXRBaseAnchorComponent::FromSpace<UOculusXRSpaceContainerAnchorComponent>(AnchorHandle, Outer);
		case EOculusXRSpaceComponentType::Sharable:
			return UOculusXRBaseAnchorComponent::FromSpace<UOculusXRSharableAnchorComponent>(AnchorHandle, Outer);
		case EOculusXRSpaceComponentType::Storable:
			return UOculusXRBaseAnchorComponent::FromSpace<UOculusXRStorableAnchorComponent>(AnchorHandle, Outer);
		default:
			return nullptr;
	}
}

OculusXRAnchors::FOculusXRAnchors* FOculusXRAnchorsModule::GetOculusAnchors()
{
	FOculusXRAnchorsModule& Module = FModuleManager::LoadModuleChecked<FOculusXRAnchorsModule>(TEXT("OculusXRAnchors"));
	return &Module.Anchors;
}

#endif // OCULUS_ANCHORS_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE(FOculusXRAnchorsModule, OculusXRAnchors)

#undef LOCTEXT_NAMESPACE
