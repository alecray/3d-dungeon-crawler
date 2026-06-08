#pragma once

#include "CoreMinimal.h"
#include "MonsterCharacter.h"
#include "BossMonster.generated.h"

class UStaticMeshComponent;
class AProjectile;
class ABubbleHazard;
class USkeletalMesh;
class UAnimSequence;
class AAttackTelegraph;

/**
 * Oversized boss built on top of AMonsterCharacter (it inherits the chase/attack/health/hit-react).
 * Tier 0-3 difficulty; each tier multiplies base stats. Fight is structured as MaxPhases "lives":
 * bringing the boss to 0 HP advances to the next phase (full heal + stat buff) rather than killing it;
 * only the final phase death actually ends the encounter.
 *
 * Tier 0: 2 phases. Phase 1 = scuttle/lunge + back weak point. Phase 2 = Ground Slam unlocked.
 * Tier 1+: reserved (see TODO below); stat scaling already works for those tiers.
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

	virtual bool IsBoss() const override { return true; }

	/** Stable identifier for this boss type — keys the "intro already seen" save flag. */
	FName GetBossId() const { return BossId; }

	/** Plays the spawn/intro animation; the boss stays frozen (no chase/attacks) until it finishes. */
	void PlayIntro();

	/** True while the spawn/intro animation is playing. */
	bool IsIntroPlaying() const { return bIntroPlaying; }
	/** Length (s) of the spawn/intro the boss freezes for — the spawn anim's length, else IntroDuration. */
	float GetIntroDuration() const { return IntroDuration; }

	// ---- Tier system (0-3) ----

	/**
	 * Sets the difficulty tier (clamped 0..3) and recomputes base stats and MaxPhases.
	 * Called by the arena right after spawning, before PlayIntro, using the player's selected tier.
	 * Tier 0 = current difficulty + 2 phases. Higher tiers multiply stats and may add phases.
	 */
	void SetTier(int32 InTier);

	/** Returns the current tier (0-3). */
	int32 GetTier() const { return Tier; }

	/**
	 * Total number of lives / phases for this fight at the current tier.
	 * Bringing HP to 0 consumes one life (and heals back to full) until the last life is gone.
	 */
	int32 GetMaxPhases() const { return MaxPhases; }

	/**
	 * Remaining lives (starts equal to MaxPhases, decrements by 1 each time HP hits 0 mid-fight).
	 * The health bar widget reads this to display star-lives.
	 */
	int32 GetPhasesRemaining() const { return PhasesRemaining; }

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

	// ---- Special mechanics ----
	UPROPERTY(EditAnywhere, Category = "Boss")
	float SpecialMinInterval = 4.f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float SpecialMaxInterval = 7.f;

	// ---- Ground Slam (phase 2+, tier 0) ----

	/** Radius (cm) of the ground-slam AOE. Jump to dodge — damage only lands if the player is grounded. */
	UPROPERTY(EditAnywhere, Category = "Boss|GroundSlam")
	float SlamRadius = 600.f;

	UPROPERTY(EditAnywhere, Category = "Boss|GroundSlam")
	float SlamDamage = 30.f;

	/** How long the telegraph disc glows before impact (seconds). Gives the player time to jump. */
	UPROPERTY(EditAnywhere, Category = "Boss|GroundSlam")
	float SlamWindupSeconds = 1.8f;

	/** How many adds a summon spawns (reserved for tier 1+). */
	UPROPERTY(EditAnywhere, Category = "Boss")
	int32 SummonCount = 2;

	// ---- Tier scaling ----

	/**
	 * Stat multiplier per tier: MaxHealth and AttackDamage are scaled by (1 + TierStatScale * Tier).
	 * At tier 0 scale = 1.0 (no change). At tier 1 = 1+TierStatScale, tier 2 = 1+2*TierStatScale, etc.
	 */
	UPROPERTY(EditAnywhere, Category = "Boss|Tier")
	float TierStatScale = 0.5f;

	// ---- Phase-1 back weak point ----
	/** Damage multiplier applied to hits landed on the boss's back during phase 1. */
	UPROPERTY(EditAnywhere, Category = "Boss|WeakPoint")
	float WeakPointMultiplier = 2.f;

	// ---- Projectile volley (tier 1+) ----
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

	// ---- Shell retreat (tier 1+) ----
	/** How long the boss tucks into its shell (invulnerable, immobile) before re-emerging. */
	UPROPERTY(EditAnywhere, Category = "Boss|Shell")
	float ShellRetreatDuration = 3.f;

	// ---- Bubble hazards (tier 1+) ----
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

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float PreferredScuttleDist = 850.f; // orbit the player around this distance while scuttling

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float ScuttleTimeMin = 2.2f;    // how long it circles before committing to a rush (longer = more scuttle)

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float ScuttleTimeMax = 3.6f;

	UPROPERTY(EditAnywhere, Category = "Boss|Movement")
	float RetreatTime = 1.1f;       // how long it backs off after a rush before scuttling again

private:
	// ---- Phase / tier internals ----
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

	// ---- Hermit-crab body ----
	// Preferred: an animated skeletal mesh + idle/run/attack anims (set these once the rig is imported).
	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMeshPath = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/SK_Hermit_Crab_Boss.SK_Hermit_Crab_Boss")));

	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	float SkeletalMeshScale = 2.25f;

	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<UAnimSequence> IdleAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/A_Hermit_Crab_Boss_Idle.A_Hermit_Crab_Boss_Idle")));

	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<UAnimSequence> RunAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/A_Hermit_Crab_Boss_Walk.A_Hermit_Crab_Boss_Walk")));

	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<UAnimSequence> AttackAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/A_Hermit_Crab_Boss_Attack.A_Hermit_Crab_Boss_Attack")));

	/** Spawn/intro animation (the rise-out-of-the-ground roar) played by PlayIntro. The intro freeze is
	    synced to its length. Empty/missing -> falls back to the procedural body-scale "pop". */
	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<UAnimSequence> SpawnAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/A_Hermit_Crab_Boss_Spawn.A_Hermit_Crab_Boss_Spawn")));

	/** Death animation — played once on death instead of the code sink/spin. */
	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<UAnimSequence> DeathAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/A_Hermit_Crab_Boss_Death.A_Hermit_Crab_Boss_Death")));

	/** Flinch/hit-react animation — played on taking damage (no-op until the asset exists). */
	UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
	TSoftObjectPtr<UAnimSequence> FlinchAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Enemies/Bosses/Hermit_Crab/A_Hermit_Crab_Boss_Flinch.A_Hermit_Crab_Boss_Flinch")));

	UPROPERTY() TObjectPtr<UAnimSequence> SpawnAnim; // loaded from SpawnAnimPath on first PlayIntro

	// ---- Tier / phase state ----
	int32 Tier = 0;
	int32 MaxPhases = 2;       // computed by SetTier (tier 0 = 2 lives)
	int32 PhasesRemaining = 2; // decremented on each HP depletion except the last
	int32 CurrentPhase = 0;    // which phase we're in (1-indexed, advances alongside lives lost)

	// Saved base stats so SetTier can scale from originals (not re-scale an already-scaled value).
	float BaseMonsterMaxHealth = 0.f;
	float BaseAttackDamage = 0.f;

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

	// Slam attack state machine.
	bool bSlamPending = false;         // wind-up in progress — waiting to detonate
	float SlamDetonateTime = 0.f;      // world time when the slam lands
	FVector SlamCenter = FVector::ZeroVector; // frozen at wind-up start (boss position)
	UPROPERTY() TObjectPtr<AAttackTelegraph> SlamTelegraph; // kept so it lives through the wind-up

	// Scuttle/lunge movement state machine (see TickCustomChase).
	enum class EMoveState : uint8 { Scuttle, Telegraph, Lunge, Retreat };
	EMoveState MoveState = EMoveState::Scuttle;
	float MoveTimer = 0.f;
	float StrafeSign = 1.f;

	/** True while we're steering directly (have line of sight); false while navmesh-pathing around cover. */
	bool bUsingDirectMove = false;
};
