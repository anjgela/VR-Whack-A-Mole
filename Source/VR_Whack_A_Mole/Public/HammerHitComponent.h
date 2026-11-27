// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WhackTypes.h"
#include "HammerHitComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHammerHitSignature, const FHitData&, HitData);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VR_WHACK_A_MOLE_API UHammerHitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHammerHitComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Assign the SceneComponent/Mesh representing the striking face of the hammer head */
	UFUNCTION(BlueprintCallable, Category = "Whack Physics")
	void SetHammerHeadComponent(USceneComponent* InHammerHead);

	/** Reset tracking history (call when the hammer is grabbed, teleported, or respawned) */
	UFUNCTION(BlueprintCallable, Category = "Whack Physics")
	void ResetTracking();

	UFUNCTION(BlueprintPure, Category = "Whack Physics")
	FVector GetSmoothedVelocity() const { return SmoothedVelocity; }

	UFUNCTION(BlueprintPure, Category = "Whack Physics")
	float GetSmoothedSpeed() const { return SmoothedVelocity.Size(); }

	/** Fired whenever a valid strike exceeds MinHitSpeed and passes cooldown/anti-cheese checks */
	UPROPERTY(BlueprintAssignable, Category = "Whack Physics")
	FOnHammerHitSignature OnHammerHit;

	// --- Tunable Parameters (Record changes in your tuning log!) ---

	/** Minimum hammer head speed (cm/s) required to register a hit instead of a gentle touch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whack Physics|Tuning")
	float MinHitSpeed = 120.0f;

	/** Speed (cm/s) at which HitStrength reaches 1.0 (Critical threshold) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whack Physics|Tuning")
	float MaxReferenceSpeed = 450.0f;

	/** Radius (cm) of the swept sphere used from PrevHeadPos to CurrentHeadPos to prevent tunneling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whack Physics|Tuning")
	float SweepRadius = 7.5f;

	/** Number of frames averaged to smooth out VR controller tracking jitter (3-5 recommended) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whack Physics|Tuning", meta = (ClampMin = "1", ClampMax = "10"))
	int32 VelocitySmoothingFrames = 4;

	/** Minimum time (seconds) before the same actor can be hit again (anti-cheese) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whack Physics|Tuning")
	float PerActorHitCooldown = 0.25f;

	/** Draws the sweep capsule, velocity vector, and hit normals in-world for debugging & report captures */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whack Physics|Debug")
	bool bDrawDebugSweep = false;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> HammerHeadComponent = nullptr;

	bool bHasPreviousPosition = false;
	FVector PreviousHeadLocation = FVector::ZeroVector;
	FVector SmoothedVelocity = FVector::ZeroVector;

	TArray<FVector> VelocityHistory;
	int32 HistoryWriteIndex = 0;

	/** Tracks last hit timestamp per target actor to prevent multi-frame double hits */
	TMap<TWeakObjectPtr<AActor>, double> RecentHitTimestamps;

	void UpdateVelocity(const FVector& CurrentLocation, float DeltaTime);
	void PerformHitSweep(const FVector& StartLocation, const FVector& EndLocation);
	EHitQuality ClassifyHit(float HitStrength, float AngleAlignment) const;
	void PruneExpiredCooldowns(double CurrentTime);
};
