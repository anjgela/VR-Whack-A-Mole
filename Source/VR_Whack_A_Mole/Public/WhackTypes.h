#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WhackTypes.generated.h"

// Adjust these channels to match your Config/DefaultEngine.ini assignments
#define ECC_Hammer ECC_GameTraceChannel1
#define ECC_Mole   ECC_GameTraceChannel2
#define ECC_Decoy  ECC_GameTraceChannel3

UENUM(BlueprintType)
enum class EHitQuality : uint8
{
	Glancing UMETA(DisplayName = "Glancing"),
	Solid    UMETA(DisplayName = "Solid"),
	Critical UMETA(DisplayName = "Critical")
};

USTRUCT(BlueprintType)
struct FHitData
{
	GENERATED_BODY()

	/** Normalized 0.0 to 1.0+ strength based on velocity and impact angle */
	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	float HitStrength = 0.0f;

	/** Raw hammer head speed (cm/s) at impact */
	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	float ImpactSpeed = 0.0f;

	/** Dot product alignment with the ideal strike direction (0.0 to 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	float AngleAlignment = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	EHitQuality Quality = EHitQuality::Glancing;

	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	FVector ImpactLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	FVector ImpactNormal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	FVector StrikeDirection = FVector::DownVector;

	UPROPERTY(BlueprintReadOnly, Category = "Whack Physics")
	TObjectPtr<AActor> HitActor = nullptr;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UWhackableInterface : public UInterface
{
	GENERATED_BODY()
};

class IWhackableInterface
{
	GENERATED_BODY()

public:
	/** Called by UHammerHitComponent when a valid velocity-thresholded strike occurs */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Whack Physics")
	void OnWhacked(const FHitData& HitData);
};