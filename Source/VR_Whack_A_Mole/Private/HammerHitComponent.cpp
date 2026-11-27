// Fill out your copyright notice in the Description page of Project Settings.


#include "HammerHitComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UHammerHitComponent::UHammerHitComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

}


// Called when the game starts
void UHammerHitComponent::BeginPlay()
{
	Super::BeginPlay();

	// Fallback: if SetHammerHeadComponent wasn't called yet, default to the owning actor's root
	if (!HammerHeadComponent && GetOwner())
	{
		HammerHeadComponent = GetOwner()->GetRootComponent();
	}

	ResetTracking();
	
}

void UHammerHitComponent::SetHammerHeadComponent(USceneComponent* InHammerHead)
{
	HammerHeadComponent = InHammerHead;
	ResetTracking();
}

void UHammerHitComponent::ResetTracking()
{
	bHasPreviousPosition = false;
	SmoothedVelocity = FVector::ZeroVector;
	VelocityHistory.Reset();
	HistoryWriteIndex = 0;
}


// Called every frame
void UHammerHitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HammerHeadComponent || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector CurrentLocation = HammerHeadComponent->GetComponentLocation();

	if (!bHasPreviousPosition)
	{
		PreviousHeadLocation = CurrentLocation;
		bHasPreviousPosition = true;
		return;
	}

	UpdateVelocity(CurrentLocation, DeltaTime);

	const float CurrentSpeed = SmoothedVelocity.Size();
	if (CurrentSpeed >= MinHitSpeed)
	{
		PerformHitSweep(PreviousHeadLocation, CurrentLocation);
	}
	else if (bDrawDebugSweep)
	{
		DrawDebugSphere(GetWorld(), CurrentLocation, SweepRadius, 12, FColor::Silver, false, 0.0f);
	}

	PreviousHeadLocation = CurrentLocation;
}

void UHammerHitComponent::UpdateVelocity(const FVector& CurrentLocation, float DeltaTime)
{
	const FVector InstantVelocity = (CurrentLocation - PreviousHeadLocation) / DeltaTime;
	const int32 MaxSamples = FMath::Max(1, VelocitySmoothingFrames);

	if (VelocityHistory.Num() < MaxSamples)
	{
		VelocityHistory.Add(InstantVelocity);
	}
	else
	{
		VelocityHistory[HistoryWriteIndex % MaxSamples] = InstantVelocity;
	}
	HistoryWriteIndex++;

	FVector Sum = FVector::ZeroVector;
	for (const FVector& Sample : VelocityHistory)
	{
		Sum += Sample;
	}
	SmoothedVelocity = Sum / static_cast<float>(VelocityHistory.Num());
}

void UHammerHitComponent::PerformHitSweep(const FVector& StartLocation, const FVector& EndLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Mole);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Decoy);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HammerHitSweep), false, GetOwner());

	TArray<FHitResult> HitResults;
	const bool bHit = World->SweepMultiByObjectType(
		HitResults,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams
	);

	if (bDrawDebugSweep)
	{
		const FColor TraceColor = bHit ? FColor::Green : FColor::Yellow;
		DrawDebugLine(World, StartLocation, EndLocation, TraceColor, false, 0.25f, 0, 1.5f);
		DrawDebugSphere(World, EndLocation, SweepRadius, 12, TraceColor, false, 0.25f);
	}

	if (!bHit)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	PruneExpiredCooldowns(Now);

	const FVector StrikeDir = SmoothedVelocity.GetSafeNormal();
	const float RawSpeed = SmoothedVelocity.Size();

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			continue;
		}

		// Anti-cheese: skip if this actor was just hit within the cooldown window
		if (const double* LastHitTime = RecentHitTimestamps.Find(HitActor))
		{
			if ((Now - *LastHitTime) < PerActorHitCooldown)
			{
				continue;
			}
		}

		// Check alignment against the mole's Up vector (1.0 = straight down onto head, 0.0 = sideways graze)
		const FVector TargetUp = HitActor->GetActorUpVector();
		const float AngleAlignment = FMath::Clamp(FVector::DotProduct(-StrikeDir, TargetUp), 0.0f, 1.0f);

		// Effective speed along the strike-to-target axis
		const float SpeedRatio = FMath::Clamp((RawSpeed - MinHitSpeed) / FMath::Max(1.0f, MaxReferenceSpeed - MinHitSpeed), 0.0f, 1.5f);
		const float HitStrength = SpeedRatio * FMath::Lerp(0.4f, 1.0f, AngleAlignment);

		RecentHitTimestamps.Add(HitActor, Now);

		FHitData Data;
		Data.HitStrength = HitStrength;
		Data.ImpactSpeed = RawSpeed;
		Data.AngleAlignment = AngleAlignment;
		Data.Quality = ClassifyHit(HitStrength, AngleAlignment);
		Data.ImpactLocation = Hit.ImpactPoint.IsNearlyZero() ? EndLocation : FVector(Hit.ImpactPoint);
		Data.ImpactNormal = Hit.ImpactNormal;
		Data.StrikeDirection = StrikeDir;
		Data.HitActor = HitActor;

		if (bDrawDebugSweep)
		{
			DrawDebugDirectionalArrow(World, Data.ImpactLocation, Data.ImpactLocation + (StrikeDir * 25.0f), 8.0f, FColor::Red, false, 1.0f, 0, 2.0f);
		}

		// Notify target directly if it implements IWhackableInterface
		if (HitActor->GetClass()->ImplementsInterface(UWhackableInterface::StaticClass()))
		{
			IWhackableInterface::Execute_OnWhacked(HitActor, Data);
		}

		// Broadcast to Gameplay / Audio / VFX Blueprints
		OnHammerHit.Broadcast(Data);
	}
}

EHitQuality UHammerHitComponent::ClassifyHit(float HitStrength, float AngleAlignment) const
{
	if (HitStrength >= 0.85f && AngleAlignment >= 0.7f)
	{
		return EHitQuality::Critical;
	}
	if (HitStrength >= 0.35f)
	{
		return EHitQuality::Solid;
	}
	return EHitQuality::Glancing;
}

void UHammerHitComponent::PruneExpiredCooldowns(double CurrentTime)
{
	for (auto It = RecentHitTimestamps.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || (CurrentTime - It.Value()) >= PerActorHitCooldown)
		{
			It.RemoveCurrent();
		}
	}
}


