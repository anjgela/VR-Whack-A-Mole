// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include "Components/ActorComponent.h"

#include "MobileSubpassBloomComponent.generated.h"

class ISceneViewExtension;
class UWorld;

UCLASS(ClassGroup = OculusXRHMD, meta = (BlueprintSpawnableComponent))
class OCULUSXRHMD_API UMobileSubpassBloomComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobileSubpassBloomComponent();

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Bloom", meta = (ClampMin = "0.0"))
	float IntensityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Bloom")
	FLinearColor TintScale = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun")
	bool bSunMerge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun", meta = (ClampMin = "0.0"))
	float SunIntensity = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun", meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float SunSkyStartCm = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun", meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float SunSkyEndCm = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun", meta = (ClampMin = "0.0"))
	float SunHaloWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun", meta = (ClampMin = "0.0"))
	float SunCoreTightness = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Mobile Subpass Bloom|Sun", meta = (ClampMin = "0.0"))
	float SunHaloFalloff = 6.0f;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:
	bool RegisterComponentWorld();
	void UnregisterComponentWorld();
	void ReleaseActiveOwner();
	void WarnApparatusDisabledOnce();

	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> ViewExtension;
	TWeakObjectPtr<UWorld> RegisteredWorld;
	uint64 ActivationOwnerId = 0;
	bool bWarnedApparatusDisabled = false;
};
