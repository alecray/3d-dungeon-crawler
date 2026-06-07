#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumber.generated.h"

class UTextRenderComponent;

/**
 * A floating combat-damage number. Spawned at a victim's location when it takes damage; renders the
 * amount as world-space 3D text (UTextRenderComponent — no UMG asset needed), drifts upward in a small
 * arc while billboarding to the camera, then shrinks out and destroys itself. Weak-point/crit hits are
 * spawned larger and tinted. Purely code-driven graybox feedback; swap the font/material later.
 */
UCLASS()
class DUNGEONCRAWLER_API ADamageNumber : public AActor
{
	GENERATED_BODY()

public:
	ADamageNumber();

	/** Sets the displayed amount and styling. Call right after spawning. */
	void Init(float Amount, bool bWeakPoint);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "DamageNumber")
	TObjectPtr<UTextRenderComponent> Text;

private:
	float Life = 0.f;
	float MaxLife = 0.9f;
	FVector Velocity = FVector::ZeroVector;
};
