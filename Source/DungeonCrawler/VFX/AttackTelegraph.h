#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AttackTelegraph.generated.h"

class UStaticMeshComponent;

/**
 * A flat red "danger zone" disc painted on the floor to telegraph an incoming melee hit. Code-driven
 * (engine cylinder + a tinted dynamic material, like the other graybox VFX). Spawned when a delayed
 * attack swing starts, sized to the actual hit radius and timed to the hit frame: it brightens/pulses
 * during the wind-up, flashes at impact, then self-destructs. Standing in it when it flashes = you get hit.
 */
UCLASS()
class DUNGEONCRAWLER_API AAttackTelegraph : public AActor
{
	GENERATED_BODY()

public:
	AAttackTelegraph();

	/** Radius (cm) of the danger circle and how long until the blow lands (the wind-up window). */
	void Configure(float Radius, float WindupSeconds);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Disc;

	float Radius = 200.f;
	float Windup = 0.6f;
	float Elapsed = 0.f;
};
