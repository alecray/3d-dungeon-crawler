#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImpactBurst.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A short, cheap code-driven hit spark: a handful of small bits fly out and shrink, plus a brief light
 * flash, then it self-destructs. No art/Niagara — same lightweight style as ADeathPoof. Spawned on a
 * landed hit for impact feedback. Keep the bit count low so heavy AoE doesn't hitch.
 */
UCLASS()
class DUNGEONCRAWLER_API AImpactBurst : public AActor
{
	GENERATED_BODY()

public:
	AImpactBurst();

	/** Tints the bits + flash (e.g. ichor green for a crab). Call right after spawning. */
	void SetColor(const FLinearColor& Color) { Tint = Color; }

	/** Overrides the burst's shape before it spawns its parts (use with a DEFERRED spawn so it takes
	    effect in BeginPlay). Pass FlashIntensity 0 for a flash-free puff (e.g. footstep dust). */
	void Configure(int32 InCount, float InSpeed, float InBitScale, float InLife, float InFlashIntensity, float InFlashRadius)
	{
		Count = InCount; Speed = InSpeed; BitScale = InBitScale; LifeSeconds = InLife;
		FlashIntensity = InFlashIntensity; FlashRadius = InFlashRadius;
	}

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Impact") int32 Count = 4;
	UPROPERTY(EditAnywhere, Category = "Impact") float Speed = 420.f;
	UPROPERTY(EditAnywhere, Category = "Impact") float BitScale = 0.08f;
	UPROPERTY(EditAnywhere, Category = "Impact") float LifeSeconds = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Impact") float FlashIntensity = 2600.f;
	UPROPERTY(EditAnywhere, Category = "Impact") float FlashRadius = 280.f;

private:
	UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bits;
	TArray<FVector> Velocities;
	FLinearColor Tint = FLinearColor(1.f, 0.95f, 0.6f);
	float Elapsed = 0.f;
};
