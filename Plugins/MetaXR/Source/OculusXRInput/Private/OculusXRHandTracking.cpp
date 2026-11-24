// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRHandTracking.h"

#include "Animation/Skeleton.h"
#include "BoneWeights.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "IOculusXRInputModule.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CoreDelegates.h"
#include "Model.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OpenXR/OculusXROpenXRUtilities.h"
#include "OculusXRInputModule.h"
#include "OculusXRInputState.h"
#include "OculusXRSimultaneousHandsAndControllersExtensionPlugin.h"

#define OCULUS_TO_UE4_SCALE 100.0f

namespace OculusXRInput
{

	FQuat FOculusHandTracking::GetBoneRotation(const int32 ControllerIndex, const EOculusXRHandType DeviceHand, const EOculusXRBone BoneId)
	{
		FQuat Rotation = FQuat::Identity;
		if (BoneId <= EOculusXRBone::Invalid && BoneId >= EOculusXRBone::Bone_Max)
		{
			return Rotation;
		}

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			const EHandBoneId HandBoneId = ToHandBone(BoneId);
			Rotation = HandState.BoneRotations[static_cast<int32>(HandBoneId)];
		}
#endif

		return Rotation;
	}

	FTransform FOculusHandTracking::GetHandJointTransform(const int32 ControllerIndex, const EOculusXRHandType DeviceHand, const int32 JointIndex, const float WorldToMeters)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			return InputModule->GetHandTrackingOpenXRExtension()->GetHandJointTransform(DeviceHand, JointIndex, WorldToMeters);
		}
#endif
		return FTransform::Identity;
	}

	bool FOculusHandTracking::IsControllerDrivenHandDataSupported()
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem())
		{
			FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			return InputModule->GetHandTrackingOpenXRExtension()->IsControllerDrivenHandDataSupported();
		}
#endif
		return false;
	}

	float FOculusHandTracking::GetHandScale(const int32 ControllerIndex, const EOculusXRHandType DeviceHand)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			return HandState.HandScale;
		}
#endif
		return 1.0f;
	}

	EOculusXRTrackingConfidence FOculusHandTracking::GetTrackingConfidence(const int32 ControllerIndex, const EOculusXRHandType DeviceHand)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			return HandState.TrackingConfidence;
		}
#endif
		return EOculusXRTrackingConfidence::Low;
	}

	EOculusXRTrackingConfidence FOculusHandTracking::GetFingerTrackingConfidence(const int32 ControllerIndex, const EOculusXRHandType DeviceHand, const EOculusHandAxes Finger)
	{
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			return HandState.FingerConfidences[(int)Finger];
		}
		return EOculusXRTrackingConfidence::Low;
	}

	FTransform FOculusHandTracking::GetPointerPose(const int32 ControllerIndex, const EOculusXRHandType DeviceHand, const float WorldToMeters)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			FTransform PoseTransform = HandState.PointerPose;
			PoseTransform.SetLocation(PoseTransform.GetLocation() * WorldToMeters);
			return PoseTransform;
		}
#endif

		return FTransform();
	}

	bool FOculusHandTracking::IsPointerPoseValid(const int32 ControllerIndex, const EOculusXRHandType DeviceHand)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			return HandState.bIsPointerPoseValid;
		}
#endif

		return false;
	}

	bool FOculusHandTracking::IsHandTrackingEnabled()
	{
		if (OculusXR::IsOpenXRSystem())
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			return InputModule->GetHandTrackingOpenXRExtension()->IsHandTrackingActive();
		}

		return false;
	}

	bool FOculusHandTracking::IsHandDominant(const int32 ControllerIndex, const EOculusXRHandType DeviceHand)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			return HandState.bIsDominantHand;
		}
#endif

		return false;
	}

	bool FOculusHandTracking::IsHandPositionValid(int32 ControllerIndex, EOculusXRHandType DeviceHand)
	{
		if (OculusXR::IsOpenXRSystem() && DeviceHand != EOculusXRHandType::None)
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			const FOculusHandControllerState& HandState = InputModule->GetHandTrackingOpenXRExtension()->HandControllerStates[DeviceHand];
			return HandState.bIsPositionValid;
		}

		return false;
	}

	bool FOculusHandTracking::IsHandInteractionProfile(EOculusXRHandType DeviceHand)
	{
		if (DeviceHand == EOculusXRHandType::None)
		{
			return false;
		}

		if (GEngine && GEngine->XRSystem.IsValid())
		{
			EControllerHand ControllerHand = DeviceHand == EOculusXRHandType::HandLeft ? EControllerHand::Left : EControllerHand::Right;
			FString InteractionProfile;
			if (GEngine->XRSystem->GetCurrentInteractionProfile(ControllerHand, InteractionProfile))
			{
				return InteractionProfile == FString("/interaction_profiles/ext/hand_interaction_ext");
			}
		}

		return false;
	}

	bool FOculusHandTracking::GetHandSkeletalMesh(USkeletalMesh* HandSkeletalMesh, const EOculusXRHandType SkeletonType, const EOculusXRHandType MeshType, const float WorldToMeters)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (HandSkeletalMesh)
		{
			if (OculusXR::IsOpenXRSystem())
			{
				TSharedPtr<FHandMesh> HandMesh = MakeShareable(new FHandMesh);
				TSharedPtr<FHandSkeleton> HandSkeleton = MakeShareable(new FHandSkeleton);

				const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
				bool SkelResult = InputModule->GetHandTrackingOpenXRExtension()->GetSkeleton(SkeletonType, HandSkeleton);
				bool MeshResult = InputModule->GetHandTrackingOpenXRExtension()->GetMesh(MeshType, HandMesh);

				if (!SkelResult || !MeshResult)
				{
#if !WITH_EDITOR
					UE_LOG(LogOcHandTracking, Error, TEXT("Failed to get mesh or skeleton data from Oculus runtime."));
#endif
					return false;
				}

				// Create Skeletal Mesh LOD Render Data
#if WITH_EDITOR
				FSkeletalMeshLODModel* LodRenderData = new FSkeletalMeshLODModel();
				HandSkeletalMesh->GetImportedModel()->LODModels.Add(LodRenderData);
#else
				FSkeletalMeshLODRenderData* LodRenderData = new FSkeletalMeshLODRenderData();
				HandSkeletalMesh->AllocateResourceForRendering();
				HandSkeletalMesh->GetResourceForRendering()->LODRenderData.Add(LodRenderData);
#endif

				// Set default LOD Info
				FSkeletalMeshLODInfo& LodInfo = HandSkeletalMesh->AddLODInfo();
				LodInfo.ScreenSize = 0.3f;
				LodInfo.LODHysteresis = 0.2f;
				LodInfo.BuildSettings.bUseFullPrecisionUVs = true;
				LodInfo.bHasBeenSimplified = false;

				InitializeHandSkeletonOpenXR(HandSkeletalMesh, HandSkeleton, WorldToMeters);

				// Add default material as backup
				LodInfo.LODMaterialMap.Add(0);
				UMaterialInterface* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
				HandSkeletalMesh->GetMaterials().Add(DefaultMaterial);
				HandSkeletalMesh->GetMaterials()[0].UVChannelData.bInitialized = true;

				// Set skeletal mesh properties
				HandSkeletalMesh->SetHasVertexColors(true);
				HandSkeletalMesh->SetEnablePerPolyCollision(false);

				InitializeHandMeshOpenXR(HandSkeletalMesh, HandMesh, WorldToMeters);

#if WITH_EDITOR
				HandSkeletalMesh->InvalidateDeriveDataCacheGUID();
				HandSkeletalMesh->PostEditChange();
#endif

				// Create Skeleton object and merge all bones
				HandSkeletalMesh->SetSkeleton(NewObject<USkeleton>());
				HandSkeletalMesh->GetSkeleton()->MergeAllBonesToBoneTree(HandSkeletalMesh);
				HandSkeletalMesh->PostLoad();

				return true;
			}
		}
#endif
		return false;
	}

	void FOculusHandTracking::InitializeHandMeshOpenXR(USkeletalMesh* SkeletalMesh, const TSharedPtr<FHandMesh> XrMesh, const float WorldToMeters)
	{
#if WITH_EDITOR
		FSkeletalMeshLODModel* LodRenderData = &SkeletalMesh->GetImportedModel()->LODModels[0];

		// Initialize mesh section
		LodRenderData->Sections.SetNumUninitialized(1);
		new (&LodRenderData->Sections[0]) FSkelMeshSection();
		auto& MeshSection = LodRenderData->Sections[0];

		// Set default mesh section properties
		MeshSection.MaterialIndex = 0;
		MeshSection.BaseIndex = 0;
		MeshSection.NumTriangles = XrMesh->NumIndices / 3;
		MeshSection.BaseVertexIndex = 0;
		MeshSection.MaxBoneInfluences = 4;
		MeshSection.NumVertices = XrMesh->NumVertices;

		float MaxDistSq = MIN_flt;
		for (uint32_t VertexIndex = 0; VertexIndex < XrMesh->NumVertices; VertexIndex++)
		{
			FSoftSkinVertex SoftVertex;
			FMemory::Memzero(SoftVertex.InfluenceWeights);
			FMemory::Memzero(SoftVertex.InfluenceBones);

			// Update vertex data
			SoftVertex.Color = FColor::White;
			XrVector3f VertexPosition = XrMesh->VertexPositions[VertexIndex];
			XrVector3f Normal = XrMesh->VertexNormals[VertexIndex];
			SoftVertex.Position = FVector3f(VertexPosition.x, VertexPosition.z, VertexPosition.y) * WorldToMeters;
			SoftVertex.TangentZ = FVector3f(Normal.x, Normal.z, Normal.y);
			SoftVertex.TangentX = FVector3f(1.0f, 0.0f, 0.0f);
			SoftVertex.TangentY = FVector3f(0.0f, 1.0f, 0.0f); // SoftVertex.TangentZ^ SoftVertex.TangentX* SoftVertex.TangentZ.W;
			SoftVertex.UVs[0] = FVector2f(XrMesh->VertexUV0[VertexIndex].x, XrMesh->VertexUV0[VertexIndex].y);

			// Update the Bounds
			float VertexDistSq = SoftVertex.Position.SizeSquared();
			if (VertexDistSq > MaxDistSq)
				MaxDistSq = VertexDistSq;

			// Update blend weights and indices
			XrVector4f BlendWeights = XrMesh->BlendWeights[VertexIndex];
			XrVector4sFB BlendIndices = XrMesh->BlendIndices[VertexIndex];

			SoftVertex.InfluenceWeights[0] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.x;
			SoftVertex.InfluenceBones[0] = BlendIndices.x;
			SoftVertex.InfluenceWeights[1] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.y;
			SoftVertex.InfluenceBones[1] = BlendIndices.y;
			SoftVertex.InfluenceWeights[2] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.z;
			SoftVertex.InfluenceBones[2] = BlendIndices.z;
			SoftVertex.InfluenceWeights[3] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.w;
			SoftVertex.InfluenceBones[3] = BlendIndices.w;

			MeshSection.SoftVertices.Add(SoftVertex);
		}

		// Update bone map
		for (uint32 BoneIndex = 0; BoneIndex < (uint32)SkeletalMesh->GetRefSkeleton().GetNum(); BoneIndex++)
		{
			MeshSection.BoneMap.Add(BoneIndex);
		}

		// Update LOD render data
		LodRenderData->NumVertices = XrMesh->NumVertices;
		LodRenderData->NumTexCoords = 1;

		// Create index buffer
		for (uint32_t Index = 0; Index < XrMesh->NumIndices; Index++)
		{
			LodRenderData->IndexBuffer.Add(XrMesh->Indices[Index]);
		}

		// Finalize Bounds
		float MaxDist = FMath::Sqrt(MaxDistSq);
		FBoxSphereBounds Bounds;
		Bounds.Origin = FVector::ZeroVector;
		Bounds.BoxExtent = FVector(MaxDist);
		Bounds.SphereRadius = MaxDist;
		SkeletalMesh->SetImportedBounds(Bounds);

#else
		FSkeletalMeshLODRenderData* LodRenderData = &SkeletalMesh->GetResourceForRendering()->LODRenderData[0];

		// Initialize Mesh Section
		LodRenderData->RenderSections.SetNumUninitialized(1);
		new (&LodRenderData->RenderSections[0]) FSkelMeshRenderSection();
		auto& MeshSection = LodRenderData->RenderSections[0];

		// Initialize render section properties
		MeshSection.MaterialIndex = 0;
		MeshSection.BaseIndex = 0;
		MeshSection.NumTriangles = XrMesh->NumIndices / 3;
		MeshSection.BaseVertexIndex = 0;
		MeshSection.MaxBoneInfluences = 4;
		MeshSection.NumVertices = XrMesh->NumVertices;
		MeshSection.bCastShadow = true;
		MeshSection.bDisabled = false;
		MeshSection.bRecomputeTangent = false;

		// Initialize Vertex Buffers
		LodRenderData->StaticVertexBuffers.PositionVertexBuffer.Init(XrMesh->NumVertices);
		LodRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.Init(XrMesh->NumVertices, 1);
		LodRenderData->StaticVertexBuffers.ColorVertexBuffer.Init(XrMesh->NumVertices);

		// Initialize Skin Weights
		TArray<FSkinWeightInfo> InWeights;
		InWeights.AddUninitialized(XrMesh->NumVertices);

		float MaxDistSq = MIN_flt;
		TMap<int32, TArray<int32>> OverlappingVertices;
		for (uint32_t VertexIndex = 0; VertexIndex < XrMesh->NumVertices; VertexIndex++)
		{
			FMemory::Memzero(InWeights[VertexIndex].InfluenceWeights);
			FMemory::Memzero(InWeights[VertexIndex].InfluenceBones);
			// Initialize vertex data
			FModelVertex ModelVertex;

			// Update Model Vertex
			XrVector3f VertexPosition = XrMesh->VertexPositions[VertexIndex];
			XrVector3f Normal = XrMesh->VertexNormals[VertexIndex];
			ModelVertex.Position = FVector3f(VertexPosition.x, VertexPosition.z, VertexPosition.y) * WorldToMeters;
			ModelVertex.TangentZ = FVector3f(Normal.x, Normal.z, Normal.y);
			ModelVertex.TangentX = FVector3f(1.0f, 0.0f, 0.0f);
			ModelVertex.TexCoord = FVector2f(XrMesh->VertexUV0[VertexIndex].x, XrMesh->VertexUV0[VertexIndex].y);

			// Add Model Vertex data to vertex buffer
			LodRenderData->StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex) = ModelVertex.Position;
			LodRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(VertexIndex, ModelVertex.TangentX, ModelVertex.GetTangentY(), ModelVertex.TangentZ);
			LodRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(VertexIndex, 0, ModelVertex.TexCoord);

			// Update the Bounds
			float VertexDistSq = ModelVertex.Position.SizeSquared();
			if (VertexDistSq > MaxDistSq)
				MaxDistSq = VertexDistSq;

			// Set vertex blend weights and indices
			TArray<int32> Vertices;
			XrVector4f BlendWeights = XrMesh->BlendWeights[VertexIndex];
			XrVector4sFB BlendIndices = XrMesh->BlendIndices[VertexIndex];

			InWeights[VertexIndex].InfluenceWeights[0] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.x;
			InWeights[VertexIndex].InfluenceBones[0] = BlendIndices.x;
			Vertices.Add(BlendIndices.x);
			InWeights[VertexIndex].InfluenceWeights[1] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.y;
			InWeights[VertexIndex].InfluenceBones[1] = BlendIndices.y;
			Vertices.Add(BlendIndices.y);
			InWeights[VertexIndex].InfluenceWeights[2] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.z;
			InWeights[VertexIndex].InfluenceBones[2] = BlendIndices.z;
			Vertices.Add(BlendIndices.z);
			InWeights[VertexIndex].InfluenceWeights[3] = UE::AnimationCore::MaxRawBoneWeightFloat * BlendWeights.w;
			InWeights[VertexIndex].InfluenceBones[3] = BlendIndices.w;
			Vertices.Add(BlendIndices.w);

			OverlappingVertices.Add(VertexIndex, Vertices);
		}

		// Update bone map for mesh section
		for (uint32 BoneIndex = 0; BoneIndex < (uint32)SkeletalMesh->GetRefSkeleton().GetNum(); BoneIndex++)
		{
			MeshSection.BoneMap.Add(BoneIndex);
		}

		// Finalize Bounds
		float MaxDist = FMath::Sqrt(MaxDistSq);
		FBoxSphereBounds Bounds;
		Bounds.Origin = FVector::ZeroVector;
		Bounds.BoxExtent = FVector(MaxDist);
		Bounds.SphereRadius = MaxDist;
		SkeletalMesh->SetImportedBounds(Bounds);

		// Assign skin weights to vertex buffer
		LodRenderData->SkinWeightVertexBuffer = InWeights;
		MeshSection.DuplicatedVerticesBuffer.Init(XrMesh->NumVertices, OverlappingVertices);

		// Set index buffer
		LodRenderData->MultiSizeIndexContainer.CreateIndexBuffer(sizeof(uint16_t));
		for (uint32_t Index = 0; Index < XrMesh->NumIndices; Index++)
		{
			LodRenderData->MultiSizeIndexContainer.GetIndexBuffer()->AddItem(XrMesh->Indices[Index]);
		}
#endif
	}

	void FOculusHandTracking::InitializeHandSkeletonOpenXR(USkeletalMesh* SkeletalMesh, const TSharedPtr<FHandSkeleton> Skeleton, const float WorldToMeters)
	{
		SkeletalMesh->GetRefSkeleton().Empty(Skeleton->NumBones);

#if WITH_EDITOR
		FSkeletalMeshLODModel* LodRenderData = &SkeletalMesh->GetImportedModel()->LODModels[0];
#else
		FSkeletalMeshLODRenderData* LodRenderData = &SkeletalMesh->GetResourceForRendering()->LODRenderData[0];
#endif
		SkeletalMesh->SetHasVertexColors(true);

		checkf(Skeleton->NumBones <= static_cast<unsigned int>(TNumericLimits<uint8>::Max()), TEXT("Bone indices are stored as uint8 type."));
		for (uint8 BoneIndex = 0; BoneIndex < static_cast<uint8>(Skeleton->NumBones); BoneIndex++)
		{
			LodRenderData->ActiveBoneIndices.Add(BoneIndex);
			LodRenderData->RequiredBones.Add(BoneIndex);

			FText BoneDisplayName;
			if (!FindBoneDisplayName(BoneDisplayName, BoneIndex))
			{
				UE_LOG(LogOcHandTracking, Error, TEXT("Cannot find bone display name for bone index: %d."), BoneIndex)
				continue;
			}
			FString BoneString = BoneDisplayName.ToString();
			FName BoneName = FName(*BoneString);

			FTransform Transform = FTransform::Identity;
			FVector BonePosition = HandBoneVectorToFVector(Skeleton->Bones[BoneIndex].Pose.position, WorldToMeters);
			FQuat BoneRotation = BoneIndex == 0 ? FQuat(-1.0f, 0.0f, 0.0f, 1.0f) : HandBoneQuatToFQuat(Skeleton->Bones[BoneIndex].Pose.orientation);

			Transform.SetLocation(BonePosition);
			Transform.SetRotation(BoneRotation);

			FReferenceSkeletonModifier Modifier = FReferenceSkeletonModifier(SkeletalMesh->GetRefSkeleton(), nullptr);
			int32 ParentIndex = -1;
			if (BoneIndex > 0)
			{
				if (Skeleton->Bones[BoneIndex].ParentBoneIndex == static_cast<int32>(EHandBoneId::Invalid))
				{
					ParentIndex = 0;
				}
				else
				{
					ParentIndex = Skeleton->Bones[BoneIndex].ParentBoneIndex;
				}
			}
			Modifier.Add(FMeshBoneInfo(BoneName, BoneString, ParentIndex), Transform);
		}
		SkeletalMesh->CalculateInvRefMatrices();
	}

	TArray<FOculusXRCapsuleCollider> FOculusHandTracking::InitializeHandPhysics(const EOculusXRHandType SkeletonType, USkinnedMeshComponent* HandComponent, const float WorldToMeters)
	{
		TArray<FOculusXRCapsuleCollider> CollisionCapsules;
		if (OculusXR::IsOpenXRSystem())
		{
			TSharedPtr<FHandSkeleton> HandSkeleton = MakeShareable(new FHandSkeleton);
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			if (!InputModule->GetHandTrackingOpenXRExtension()->GetSkeleton(SkeletonType, HandSkeleton))
			{
#if !WITH_EDITOR
				UE_LOG(LogOcHandTracking, Error, TEXT("Failed to get skeleton data from Oculus runtime."));
#endif
				return CollisionCapsules;
			}
#endif
			TArray<UPrimitiveComponent*> IgnoreCapsules;
			CollisionCapsules.AddDefaulted(HandSkeleton->NumBoneCapsules);
			for (uint32 CapsuleIndex = 0; CapsuleIndex < HandSkeleton->NumBoneCapsules; CapsuleIndex++)
			{
				FHandBoneCapsule XrBoneCapsule = HandSkeleton->BoneCapsules[CapsuleIndex];

				UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(HandComponent);

				FVector CapsulePointZero = HandBoneVectorToFVector(XrBoneCapsule.Points[0], WorldToMeters);
				FVector CapsulePointOne = HandBoneVectorToFVector(XrBoneCapsule.Points[1], WorldToMeters);
				FVector Delta = (CapsulePointOne - CapsulePointZero);

				FName BoneName = HandComponent->GetSkinnedAsset()->GetRefSkeleton().GetBoneName(XrBoneCapsule.BoneIndex);

				float CapsuleHeight = Delta.Size();
				float CapsuleRadius = XrBoneCapsule.Radius * WorldToMeters;

				Capsule->SetCapsuleRadius(CapsuleRadius);
				Capsule->SetCapsuleHalfHeight(Delta.Size() / 2 + CapsuleRadius);
				Capsule->SetupAttachment(HandComponent, BoneName);
				Capsule->SetCollisionProfileName(HandComponent->GetCollisionProfileName());
				Capsule->RegisterComponentWithWorld(HandComponent->GetWorld());
				Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				FRotator CapsuleRotation = FQuat::FindBetweenVectors(FVector::RightVector, Delta).Rotator() + FRotator(0, 0, 90);
				;

				Capsule->SetRelativeRotation(CapsuleRotation);
				Capsule->SetRelativeLocation(CapsulePointZero + (Delta / 2));

				CollisionCapsules[CapsuleIndex].Capsule = Capsule;
				CollisionCapsules[CapsuleIndex].BoneId = (EOculusXRBone)XrBoneCapsule.BoneIndex;

				IgnoreCapsules.Add(Capsule);
			}

			for (int32 CapsuleIndex = 0; CapsuleIndex < CollisionCapsules.Num(); CapsuleIndex++)
			{
				CollisionCapsules[CapsuleIndex].Capsule->MoveIgnoreComponents = IgnoreCapsules;
			}

			return CollisionCapsules;
		}
		return CollisionCapsules;
	}

	EHandBoneId FOculusHandTracking::ToHandBone(EOculusXRBone Bone)
	{
		if (Bone > EOculusXRBone::Bone_Max)
			return EHandBoneId::Invalid;

		return static_cast<EHandBoneId>(Bone);
	}

	FString FOculusHandTracking::GetBoneName(const uint8 Bone)
	{
		FText DisplayName;
		if (FindBoneDisplayName(DisplayName, Bone))
		{
			return DisplayName.ToString();
		}
		if (FindBoneDisplayName(DisplayName, static_cast<uint8>(EOculusXRBone::Invalid)))
		{
			return DisplayName.ToString();
		}
		return { "Invalid" };
	}

	bool FOculusHandTracking::FindBoneDisplayName(FText& DisplayName, uint8 Bone)
	{
		return StaticEnum<EOculusXRBone>()->FindDisplayNameTextByValue(DisplayName, Bone);
	}

	FVector FOculusHandTracking::HandBoneVectorToFVector(XrVector3f XrVector, float WorldToMeters)
	{
		return FVector(XrVector.x, -XrVector.y, XrVector.z) * WorldToMeters;
	}

	FQuat FOculusHandTracking::HandBoneQuatToFQuat(XrQuaternionf XrQuat)
	{
		return FQuat(XrQuat.x, -XrQuat.y, XrQuat.z, -XrQuat.w);
	}

	EOculusXRControllerDrivenHandPoseTypes FOculusHandTracking::ControllerDrivenHandType = EOculusXRControllerDrivenHandPoseTypes::None;

	void FOculusHandTracking::SetControllerDrivenHandPoses(const EOculusXRControllerDrivenHandPoseTypes Type)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		if (OculusXR::IsOpenXRSystem())
		{
			FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			if (InputModule->GetHandTrackingOpenXRExtension()->SetControllerDrivenHandPoses(Type))
			{
				FOculusHandTracking::ControllerDrivenHandType = Type;
			}
		}
#endif
	}

	bool FOculusHandTracking::SetSimultaneousHandsAndControllersEnabled(bool bEnabled)
	{
		if (OculusXR::IsOpenXRSystem())
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			return InputModule->GetSimultaneousHandsAndControllersEOpenXRExtension()->SetSimultaneousHandsAndControllersEnabled(bEnabled);
		}

		return false;
	}

	bool FOculusHandTracking::IsSimultaneousHandsAndControllersEnabled()
	{
		if (OculusXR::IsOpenXRSystem())
		{
			const FOculusXRInputModule* InputModule = static_cast<FOculusXRInputModule*>(&FOculusXRInputModule::Get());
			return InputModule->GetSimultaneousHandsAndControllersEOpenXRExtension()->IsSimultaneousHandsAndControllersEnabled();
		}

		return false;
	}
} // namespace OculusXRInput
