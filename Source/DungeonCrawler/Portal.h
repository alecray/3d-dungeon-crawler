#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A travel portal. Interacting with it (E) saves the profile and opens the target level. Rendered as
 * a procedural "energy gate": a glowing vertical disc, a ring of shards that spins, and a pulsing
 * light (all code-built, no art). Used for the town's enter-dungeon portal and the dungeon's return
 * portals; can start dormant (hidden) and be switched on later (e.g. after the boss dies).
 */
UCLASS()
class DUNGEONCRAWLER_API APortal : public AActor
{
	GENERATED_BODY()

public:
	APortal();

	virtual void Tick(float DeltaSeconds) override;

	/** Short level name to open on use (e.g. "L_Town" or "L_DungeonTest"). */
	UPROPERTY(EditAnywhere, Category = "Portal")
	FName TargetMapName;

	/** A portal can be placed but dormant; activate it later (the after-boss return portal). */
	UPROPERTY(EditAnywhere, Category = "Portal")
	bool bActive = true;

	FName GetTargetMapName() const { return TargetMapName; }
	bool IsActive() const { return bActive; }
	void SetActive(bool bInActive);
	void SetTargetMapName(FName InMap) { TargetMapName = InMap; }

protected:
	virtual void BeginPlay() override;

	/** Builds a tinted, unlit-looking dynamic material for the portal's emissive parts. */
	void ApplyGlowMaterial(UStaticMeshComponent* Comp);

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<USceneComponent> Root;

	/** The glowing portal surface (a thin vertical disc); also the Visibility target for [E]. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Disc;

	/** Spins the ring of shards. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<USceneComponent> Spinner;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UPointLightComponent> Glow;

private:
	float Elapsed = 0.f;

	static constexpr float CenterZ = 150.f;     // portal center height above the floor
	static constexpr int32 ShardCount = 12;     // shards around the ring
	static constexpr float RingRadius = 78.f;   // shard ring radius
	static constexpr float BaseIntensity = 5000.f;
};
