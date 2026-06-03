#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeathPoof.generated.h"

class UStaticMeshComponent;

/**
 * A short-lived graybox "poof": a burst of small cube particles that fly outward and shrink, then the
 * actor despawns. Spawned when an enemy dies (placeholder for a real Niagara FX later).
 */
UCLASS()
class DUNGEONCRAWLER_API ADeathPoof : public AActor
{
	GENERATED_BODY()

public:
	ADeathPoof();

	virtual void Tick(float DeltaSeconds) override;

	/** Tint the particles (e.g. to the enemy's color). */
	void SetColor(const FLinearColor& Color);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Poof")
	int32 Count = 14;

	UPROPERTY(EditAnywhere, Category = "Poof")
	float Speed = 280.f;

	UPROPERTY(EditAnywhere, Category = "Poof")
	float LifeSeconds = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Poof")
	float ParticleScale = 0.12f;

private:
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FVector> Velocities;
	FLinearColor Tint = FLinearColor(0.8f, 0.4f, 0.3f);
	float Elapsed = 0.f;
};
