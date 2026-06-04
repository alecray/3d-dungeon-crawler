#pragma once

#include "CoreMinimal.h"
#include "MonsterCharacter.h"
#include "BossMonster.generated.h"

class UStaticMeshComponent;
class AProjectile;
class ABubbleHazard;

/**
 * Oversized boss built on top of AMonsterCharacter (it inherits the chase/attack/health/hit-react).
 * Adds a 3-phase fight: as its health crosses thresholds it advances phase, morphs by revealing an
 * extra cube section, and buffs its stats. On a randomized timer it also fires a random special
 * mechanic (ground slam / summon adds / enrage burst).
 */
UCLASS()
class DUNGEONCRAWLER_API ABossMonster : public AMonsterCharacter
{
	GENERATED_BODY()

public:
	ABossMonster();

	virtual void Tick(float DeltaSeconds) override;

	/** Doubles damage from behind while the phase-1 back weak point is exposed. */
	virtual float ApplyHitDamage(float BaseDamage, const FVector& FromLocation) override;

	/** Stable identifier for this boss type — keys the "intro already seen" save flag. */
	FName GetBossId() const { return BossId; }

	/** Plays the spawn/intro animation; the boss stays frozen (no chase/attacks) until it finishes. */
	void PlayIntro();

	/** True while the spawn/intro animation is playing. */
	bool IsIntroPlaying() const { return bIntroPlaying; }

protected:
	virtual void BeginPlay() override;

	/** Crab-like movement: scuttle sideways around the player and telegraph forward lunges. */
	virtual bool TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist) override;

	/** Stable id for this boss type (persists which bosses' intros have already played). */
	UPROPERTY(EditAnywhere, Category = "Boss")
	FName BossId = TEXT("HermitCrab");

	/** Length of the spawn/intro animation (seconds). */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float IntroDuration = 1.8f;

	/** Health fraction at/below which the boss enters phase 2 and phase 3. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase2HealthPct = 0.66f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase3HealthPct = 0.33f;

	// ---- Special mechanics ----
	UPROPERTY(EditAnywhere, Category = "Boss")
	float SpecialMinInterval = 4.f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float SpecialMaxInterval = 7.f;

	/** Ground-slam radius (cm) and damage. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float SlamRadius = 450.f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float SlamDamage = 22.f;

	/** How many adds a summon spawns. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	int32 SummonCount = 2;

	// ---- Phase-1 back weak point ----
	/** Damage multiplier applied to hits landed on the boss's back during phase 1. */
	UPROPERTY(EditAnywhere, Category = "Boss|WeakPoint")
	float WeakPointMultiplier = 2.f;

	// ---- Projectile volley ----
	UPROPERTY(EditAnywhere, Category = "Boss|Projectiles")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Boss|Projectiles")
	float ProjectileDamage = 14.f;

	/** Half-angle (deg) of the spread fired in phase 2+ (3 bolts). */
	UPROPERTY(EditAnywhere, Category = "Boss|Projectiles")
	float ProjectileSpread = 14.f;

	/** Bolt flight speed (cm/s) — slower than the player's bolts so they're dodgeable. */
	UPROPERTY(EditAnywhere, Category = "Boss|Projectiles")
	float ProjectileSpeed = 1500.f;

	// ---- Shell retreat (phase 3) ----
	/** How long the boss tucks into its shell (invulnerable, immobile) before re-emerging. */
	UPROPERTY(EditAnywhere, Category = "Boss|Shell")
	float ShellRetreatDuration = 3.f;

	// ---- Bubble hazards (phases 2-3) ----
	UPROPERTY(EditAnywhere, Category = "Boss|Bubbles")
	TSubclassOf<ABubbleHazard> BubbleClass;

	UPROPERTY(EditAnywhere, Category = "Boss|Bubbles")
	int32 BubbleCount = 4;

	UPROPERTY(EditAnywhere, Category = "Boss|Bubbles")
	float BubbleRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bubbles")
	float BubbleDamage = 12.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bubbles")
	float BubbleLifetime = 5.f;

	// ---- Lunge movement ----
	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float LungeRange = 950.f;       // start winding up a lunge once within this distance

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float LungeTelegraph = 0.45f;   // wind-up hold before the dash

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float LungeTime = 0.45f;        // dash duration

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float LungeSpeedMult = 2.6f;    // dash speed vs. base move speed

private:
	void AdvanceToPhase(int32 NewPhase);
	void ScheduleNextSpecial();
	void DoRandomSpecial();

	void SlamAttack();
	void SummonAdds();
	void EnrageBurst();
	void SpitProjectiles();
	void BubbleBurst();
	void EnterShellRetreat();
	void UpdateShellRetreat(float DeltaSeconds);

	/** True when nothing solid blocks the straight line from the boss to the player. */
	bool HasLineOfSightToPlayer(AActor* Player) const;

	/** Three cube "growths" revealed one-per-phase to show the boss morphing. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> MorphParts;

	/** Bright marker on the boss's back, exposed during phase 1 to flag the weak point. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BackWeakMesh;

	/** Imported hermit-crab body mesh; when valid it replaces the graybox cubes. */
	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> CrabMesh;

	/** Yaw applied to the crab mesh so its forward faces the actor's forward (tweak per imported asset). */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float CrabMeshYaw = -90.f;

	/** Target size (cm) of the crab's largest dimension — scales the imported mesh to a boss-sized body. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float CrabMeshTargetSize = 585.f;

	int32 CurrentPhase = 0;
	float NextSpecialTime = 0.f;
	float EnrageEndTime = 0.f;
	float BaseMoveSpeed = 0.f;

	/** True while the phase-1 back weak point is exposed. */
	bool bBackWeakActive = false;

	// Spawn/intro animation state (boss is frozen while this plays).
	void UpdateIntro(float DeltaSeconds);
	bool bIntroPlaying = false;
	float IntroTimeLeft = 0.f;

	// Shell-retreat state (boss is frozen + invulnerable while this is active).
	bool bShellRetreat = false;
	float ShellTimeLeft = 0.f;

	// Scuttle/lunge movement state machine (see TickCustomChase).
	enum class EMoveState : uint8 { Scuttle, Telegraph, Lunge };
	EMoveState MoveState = EMoveState::Scuttle;
	float MoveTimer = 0.f;
	float StrafeSign = 1.f;

	/** True while we're steering directly (have line of sight); false while navmesh-pathing around cover. */
	bool bUsingDirectMove = false;
};
