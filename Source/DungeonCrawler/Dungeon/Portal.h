#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A travel portal. Interacting with it (E) saves the profile and opens the target level. Rendered as
 * a procedural "energy gate": a glowing disc surrounded by three shard rings spinning at different
 * speeds, each ring's shards sinusoidally offset in Z to form a helix/vortex silhouette (all
 * code-built, no art). Can start dormant and be switched on later (e.g. after boss death).
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

private:
	bool bIsReturnPortal = false;

public:

	/** Portal energy color — tints the disc, all shard rings, and the point light. */
	UPROPERTY(EditAnywhere, Category = "Portal|Appearance")
	FLinearColor TintColor = FLinearColor(0.25f, 0.85f, 1.0f);

	/** Global speed multiplier for ring spin and Z-wave helix motion. */
	UPROPERTY(EditAnywhere, Category = "Portal|Appearance", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float SwirlySpeed = 1.0f;

	FName GetTargetMapName() const { return TargetMapName; }
	bool IsActive() const { return bActive; }
	void SetActive(bool bInActive);
	void SetTargetMapName(FName InMap) { TargetMapName = InMap; }

	/** True for portals spawned inside the dungeon to return the player to town (set by ABossArena).
	 *  When true the player controller shows a "Leave dungeon?" confirm instead of the map/tier select. */
	bool IsReturnPortal() const { return bIsReturnPortal; }
	void SetIsReturnPortal(bool bVal) { bIsReturnPortal = bVal; }

protected:
	virtual void BeginPlay() override;

	void ApplyGlowMaterial(UStaticMeshComponent* Comp, float BrightnessMult = 1.0f);

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<USceneComponent> Root;

	/** The glowing portal surface (thin vertical disc); also the Visibility target for [E]. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Disc;

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UPointLightComponent> Glow;

	// Three shard rings: inner (fast/bright), middle, outer (slow/dim). Shards are positioned and
	// rotated per-frame in Tick so they can carry a helical Z-wave offset independently.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Ring0;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Ring1;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Ring2;

private:
	float Elapsed = 0.f;

	static constexpr float CenterZ       = 150.f;
	static constexpr float BaseIntensity = 2600.f;
};
