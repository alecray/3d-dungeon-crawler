#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInstanceDynamic;

/**
 * A graybox projectile (crossbow bolt / spell). Flies straight, damages the first monster it hits
 * via UHealthComponent, and destroys on any blocking hit. Spawn with Launch().
 */
UCLASS()
class DUNGEONCRAWLER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	/**
	 * Sets damage + who fired it (ignored for collision) and sends it flying along Direction.
	 * @param bTargetPlayer  when true the bolt damages the player (and ignores monsters) — used by the
	 *                       boss; when false it damages monsters — used by the player's weapons.
	 * @param GravityScale   0 = straight flight; >0 makes it arc (lobbed shots).
	 * @param InSpeed        flight speed in cm/s; <0 uses the class default Speed.
	 */
	void Launch(const FVector& Direction, float InDamage, AActor* Shooter, bool bTargetPlayer = false, float GravityScale = 0.f, float InSpeed = -1.f);

	/**
	 * Switches this bolt to the fireball look (mage casts only — crossbow/boss bolts stay plain).
	 * Tier 0 (now): emissive orange orb + a flickering point light, fully code-driven (no assets).
	 * Tier 1 (auto): if an authored Niagara system exists at /Game/VFX/NS_Fireball it replaces the orb,
	 *                and /Game/VFX/NS_FireballImpact (if present) spawns on hit. See TODO "fireball art".
	 */
	void EnableFireballVisual();

protected:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float Speed = 2600.f;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float LifeSeconds = 4.f;

	// ---- Fireball visual (enabled per-shot via EnableFireballVisual; off for plain bolts) ----
	UPROPERTY(VisibleAnywhere, Category = "Fireball")
	TObjectPtr<UPointLightComponent> FireLight;

	UPROPERTY(VisibleAnywhere, Category = "Fireball")
	TObjectPtr<UNiagaraComponent> FireFX;

	/** Authored later: in-flight fireball system. Soft-loaded from /Game/VFX/NS_Fireball (null until it exists). */
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> FireballSystem;

	/** Authored later: hit burst. Soft-loaded from /Game/VFX/NS_FireballImpact (null until it exists). */
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> ImpactFX;

private:
	float Damage = 10.f;
	bool bDamagesPlayer = false; // false: hits monsters (player's shot); true: hits the player (boss shot)

	bool bFireball = false;        // this bolt uses the fireball visuals
	bool bUsingNiagara = false;    // Tier 1 system present (skips the Tier 0 orb/flicker)
	float FireElapsed = 0.f;       // drives the Tier 0 light flicker
	float BaseLightIntensity = 0.f;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OrbMID; // Tier 0 emissive orb material
};
