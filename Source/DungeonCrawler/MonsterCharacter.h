#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterCharacter.generated.h"

class UStaticMeshComponent;
class UHealthComponent;

/**
 * Graybox melee monster. Built from cube primitives, auto-possessed by an AIController, and driven
 * by simple steering in Tick: chase the player once within AggroRange, then deal damage on a
 * cooldown once within AttackRange. Has a HealthComponent and collapses when it dies.
 */
UCLASS()
class DUNGEONCRAWLER_API AMonsterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMonsterCharacter();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<UHealthComponent> Health;

	// ---- Graybox body ----
	/** Parent of all body meshes; scaled for the hit-react pop so the whole monster reacts uniformly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<USceneComponent> BodyRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> LeftArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> RightArmMesh;

	// ---- Tunables ----
	/** Start chasing the player within this distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float AggroRange = 1600.f;

	/** Close enough to swing at the player (cm). */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float AttackRange = 170.f;

	/** Damage dealt to the player per hit. */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float AttackDamage = 12.f;

	/** Seconds between attacks. */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float AttackCooldown = 1.4f;

	UPROPERTY(EditAnywhere, Category = "Monster")
	float MoveSpeed = 320.f;

	/** Starting/max health for this monster. */
	UPROPERTY(EditAnywhere, Category = "Monster", meta = (ClampMin = "1"))
	float MonsterMaxHealth = 45.f;

	/** How long the hit-react pop lasts after taking damage (s). */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float HitReactDuration = 0.18f;

	/** Peak extra scale of the body during the hit-react pop (0.35 = +35%). */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float HitReactScale = 0.35f;

	/** Uniform resting scale of BodyRoot (subclasses bump this to make a bigger monster). */
	float BodyScale = 1.f;

	/** Builds one graybox cube under BodyRoot. Available to subclasses (e.g. boss morph parts). */
	UStaticMeshComponent* AddBox(const TCHAR* Name, const FVector& Center, const FVector& SizeCm);

	/** Plays the hit-react pop without applying damage (for special-attack feedback). */
	void TriggerHitReact() { HitReactTimeLeft = HitReactDuration; }

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

private:
	void HandleDeath(UHealthComponent* DeadComponent);
	void HandleDamaged(UHealthComponent* DamagedComponent, float Amount);

	float LastAttackTime = -1000.f;
	float HitReactTimeLeft = 0.f;
	bool bDead = false;
};
