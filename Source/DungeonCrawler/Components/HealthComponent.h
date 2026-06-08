#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UHealthComponent;

/** Fired once when health first reaches zero. Passes the component that died. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthDepleted, UHealthComponent*);

/** Fired whenever damage is actually applied. Passes the component and the damage amount. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthDamaged, UHealthComponent*, float);

/**
 * Simple health/damage container shared by the player and monsters. Holds current/max health,
 * applies damage, and broadcasts OnDepleted exactly once when health hits zero.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	/** Subtracts Amount (clamped). Returns the damage actually applied. No effect once dead. */
	float ApplyDamage(float Amount);

	/** Adds Amount back, clamped to MaxHealth. No effect once dead. */
	void Heal(float Amount);

	/** Overrides the max (e.g. per monster type). When bRefill, current health is set to the new max. */
	void SetMaxHealth(float NewMax, bool bRefill = true);

	/** When invulnerable, ApplyDamage is a no-op (dev god mode). */
	void SetInvulnerable(bool bInInvulnerable) { bInvulnerable = bInInvulnerable; }
	bool IsInvulnerable() const { return bInvulnerable; }

	/** Forces health to zero and fires OnDepleted once, ignoring invulnerability (dev kill). */
	void Kill();

	/** Brings a dead component back with ToHealth (clamped to [1, Max]) and clears the dead flag, so it can
	    take/heal damage again. Used by the death-scroll "cheat death" path. */
	void Revive(float ToHealth);

	bool IsDead() const { return bDead; }
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	/** Broadcast once when health reaches zero. */
	FOnHealthDepleted OnDepleted;

	/** Broadcast each time damage is applied (before OnDepleted, if this hit was fatal). */
	FOnHealthDamaged OnDamaged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float Health = 0.f;

	bool bDead = false;
	bool bInvulnerable = false;
};
