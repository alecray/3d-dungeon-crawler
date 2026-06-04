#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

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
	 */
	void Launch(const FVector& Direction, float InDamage, AActor* Shooter, bool bTargetPlayer = false, float GravityScale = 0.f);

protected:
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

private:
	float Damage = 10.f;
	bool bDamagesPlayer = false; // false: hits monsters (player's shot); true: hits the player (boss shot)
};
