#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bonfire.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A rest point (think Dark-Souls bonfire) placed in Rest rooms. Graybox: a stone base + a flickering
 * flame + a warm light. Interacting (E) fully heals the player and refills mana/stamina, and saves the
 * profile as a checkpoint. Reusable — rest as often as you like. Swap BaseMeshOverride / BowlMeshOverride
 * for finished art later without touching gameplay.
 */
UCLASS()
class DUNGEONCRAWLER_API ABonfire : public AActor
{
	GENERATED_BODY()

public:
	ABonfire();

	virtual void Tick(float DeltaSeconds) override;

	/** Heals the resting pawn to full + refills its pools + saves. Returns true if anything was restored. */
	bool Rest(APawn* ByPawn);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Bonfire")
	TObjectPtr<USceneComponent> Root;

	/** Stone base — blocks the player + the interact line-trace. */
	UPROPERTY(VisibleAnywhere, Category = "Bonfire")
	TObjectPtr<UStaticMeshComponent> Base;

	/** Graybox flame (recolored cube); swap for a VFX/mesh later. */
	UPROPERTY(VisibleAnywhere, Category = "Bonfire")
	TObjectPtr<UStaticMeshComponent> Flame;

	UPROPERTY(VisibleAnywhere, Category = "Bonfire")
	TObjectPtr<UPointLightComponent> Light;

	/** Finished-mesh swap-in points (null = keep the graybox). */
	UPROPERTY(EditAnywhere, Category = "Bonfire|Meshes")
	TObjectPtr<UStaticMesh> BaseMeshOverride;

	UPROPERTY(EditAnywhere, Category = "Bonfire|Meshes")
	TObjectPtr<UStaticMesh> BowlMeshOverride;

private:
	float FlickerPhase = 0.f;
	float BaseLightIntensity = 2400.f;
};
