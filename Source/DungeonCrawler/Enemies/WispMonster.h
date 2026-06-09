#pragma once

#include "CoreMinimal.h"
#include "MonsterCharacter.h"
#include "WispMonster.generated.h"

class UPointLightComponent;

/**
 * Wisp enemy: a supernatural orb that hovers and drifts slowly toward the player.
 * Damages via proximity aura (no melee swing); uses flying movement so gravity is disabled.
 *
 * Animation slots (same 6 as all regular enemies — assets assigned once Blender exports arrive):
 *   IdleAnimPath      — looping hover idle
 *   RunAnimPath       — looping hover drift (plays while approaching)
 *   AttackAnimPath    — reserved (aura damage is code-driven; this slot unused at runtime)
 *   DeathAnimPath     — one-shot death
 *   FlinchAnimPath    — one-shot hit-react L
 *   FlinchAnimAltPath — one-shot hit-react R
 *
 * Visual: a UPointLightComponent provides the glow; its intensity is driven each Tick by a
 * sine wave (Sin(Time × 4) remapped [0.8, 1.2] × LightBaseIntensity). Tune WispColor and
 * LightBaseIntensity in the Details panel — OnConstruction applies them live.
 * The skeletal mesh material (M_Wisp / MI_Wisp) should be Unlit with an EmissiveStrength
 * scalar parameter (default 5.0); build it with Tools/make_wisp_material.py.
 */
UCLASS()
class DUNGEONCRAWLER_API AWispMonster : public AMonsterCharacter
{
	GENERATED_BODY()

public:
	AWispMonster();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/** Replaces navmesh chase with a smooth hover-drift toward the player. */
	virtual bool TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist) override;

	/** No-op — the Wisp deals damage via proximity aura, not melee swings. */
	virtual void PlayAttackAnim() override;

	// ---- Light ----

	UPROPERTY(VisibleAnywhere, Category = "Wisp|Light")
	TObjectPtr<UPointLightComponent> WispLight;

	/** Base light intensity (cd). Flickered ±20% each Tick via sine wave. */
	UPROPERTY(EditAnywhere, Category = "Wisp|Light")
	float LightBaseIntensity = 400.f;

	/** Emissive/light color. Drives both the point light and (if set) the mesh material tint. */
	UPROPERTY(EditAnywhere, Category = "Wisp|Light")
	FLinearColor WispColor = FLinearColor(0.35f, 0.25f, 1.0f, 1.0f); // blue-purple ghost

	// ---- Movement ----

	/** Hover target: this many cm above the player's actor origin. */
	UPROPERTY(EditAnywhere, Category = "Wisp|Movement")
	float HoverHeight = 90.f;

	/** Maximum drift speed toward the hover target (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Wisp|Movement")
	float DriftSpeed = 160.f;

	// ---- Aura damage ----

	/** Radius (cm) within which the aura deals damage each interval. */
	UPROPERTY(EditAnywhere, Category = "Wisp|Aura")
	float AuraRadius = 90.f;

	/** Damage dealt to the player each interval while they are inside AuraRadius. */
	UPROPERTY(EditAnywhere, Category = "Wisp|Aura")
	float AuraDamage = 6.f;

	/** Seconds between aura damage ticks. */
	UPROPERTY(EditAnywhere, Category = "Wisp|Aura")
	float AuraDamageInterval = 0.8f;

private:
	float AuraTimer = 0.f;
};
