#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BubbleHazard.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * A lingering area hazard dropped by the boss (a bubbling pool / gas cloud, graybox). Grows in, then
 * damages any non-monster pawn (the player) standing inside it on a cadence for its lifetime, then
 * pops and destroys itself. Spawned by ABossMonster in phases 2-3. Swap the sphere mesh/material later.
 */
UCLASS()
class DUNGEONCRAWLER_API ABubbleHazard : public AActor
{
	GENERATED_BODY()

public:
	ABubbleHazard();

	/** Configures footprint, damage-per-tick, and how long the pool lasts. Call right after spawning. */
	void Init(float InRadius, float InDamage, float InLifetime);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Bubble")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Bubble")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	void DamageInside();

	float Radius = 130.f;
	float Damage = 14.f;
	float DamageInterval = 0.6f;
	float Lifetime = 5.f;
	float Age = 0.f;
	float DamageTimer = 0.f;
};
