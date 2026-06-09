#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillTypes.h" // EActiveAbility
#include "CombatComponent.generated.h"

class AFirstPersonCharacter;
class AProjectile;
class UAnimSequence;
struct FInputActionValue;

/** How the equipped weapon attacks. Set by the pawn when the held weapon changes. */
UENUM(BlueprintType)
enum class ECombatStyle : uint8
{
	Melee,
	Ranged,
	Mage
};

/**
 * The player's attack execution, extracted from AFirstPersonCharacter: melee swings (with a wall-deflect
 * bounce), ranged/mage projectiles, the style-specific active abilities, plus the hit-stop and camera-kick
 * "juice." The owning pawn binds Attack()/UseAbility() to input and calls SetStyle() when the held weapon
 * changes; everything else (stats, stamina/mana, skill tree, camera, held mesh) is read back off the owner.
 */
UCLASS(ClassGroup = (DungeonCrawler), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Bound by the owning pawn to the Attack / Ability inputs. */
	void Attack(const FInputActionValue& Value);
	void UseAbility(const FInputActionValue& Value);

	/** The active style (set by the pawn when the held weapon changes); selects melee/ranged/mage behaviour. */
	void SetStyle(ECombatStyle NewStyle) { CurrentStyle = NewStyle; }
	ECombatStyle GetStyle() const { return CurrentStyle; }

	/** Overrides the active melee swing animations when the held weapon changes. Pass an empty Alt
	 *  for single-animation weapons (Sword); pass both for L/R alternating weapons (Club). */
	void SetMeleeAnims(const TSoftObjectPtr<UAnimSequence>& Primary, const TSoftObjectPtr<UAnimSequence>& Alt);

	/** World time the player last landed a damaging hit (the HUD polls this to flash the hit marker). */
	double GetLastHitLandedTime() const { return LastHitLandedTime; }

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Combat") float MeleeStaminaCost = 10.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float RangedStaminaCost = 12.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float SpellManaCost = 18.f;

	/** Base projectile damage (scaled by Dexterity for ranged / Intelligence for mage). */
	UPROPERTY(EditAnywhere, Category = "Combat") float ProjectileDamage = 18.f;

	/** Reach of a melee swing from the camera (cm). */
	UPROPERTY(EditAnywhere, Category = "Combat") float AttackRange = 220.f;
	/** Radius of the swing's sphere sweep (cm). */
	UPROPERTY(EditAnywhere, Category = "Combat") float AttackRadius = 45.f;
	/** Damage dealt per landed swing. */
	UPROPERTY(EditAnywhere, Category = "Combat") float AttackDamage = 22.f;
	/** Minimum seconds between swings. */
	UPROPERTY(EditAnywhere, Category = "Combat") float AttackCooldown = 0.45f;

	/** Swing animation played on a melee attack. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSoftObjectPtr<UAnimSequence> SwingAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Weapons/Sword/A_Sword_Attack.A_Sword_Attack")));
	/** Deflect/bounce animation played when a swing strikes a solid, non-damageable surface (wall/prop). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSoftObjectPtr<UAnimSequence> DeflectAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Weapons/Sword/A_Sword_Deflect.A_Sword_Deflect")));
	/** Crossbow fire animation. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSoftObjectPtr<UAnimSequence> CrossbowShootAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Weapons/Crossbow/A_Crossbow_Shoot.A_Crossbow_Shoot")));
	/** Staff spell-cast animation. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSoftObjectPtr<UAnimSequence> StaffCastAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Weapons/Staff/A_Staff_Cast.A_Staff_Cast")));

	/** Alt swing animation (right-side swing for Club; when set alongside SwingAnim, attacks alternate
	 *  L/R and the matched flinch direction is signalled to the struck enemy). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSoftObjectPtr<UAnimSequence> SwingAnimAlt;

private:
	/** Owning player, cached in BeginPlay (used to reach camera, held mesh, stats/resources, skill tree). */
	UPROPERTY()
	TObjectPtr<AFirstPersonCharacter> OwnerChar;

	void MeleeAttack();
	/** The actual swing hit (sweep + damage), fired on a timer partway through the swing animation. */
	void DoMeleeHit();
	/** Plays the deflect/bounce reaction when a swing hits a solid non-damageable surface. */
	void PlayMeleeDeflect(const FVector& ImpactPoint);
	/** Fires the center bolt plus ExtraProjectiles additional bolts in a horizontal spread. */
	void FireProjectile(float Damage, int32 ExtraProjectiles = 0, bool bFireball = false);
	/** Damages every monster within Radius of the player (Whirlwind / Nova). */
	void PerformAreaBurst(float Radius, float Damage);
	/** Briefly slows global time for impact "hit-stop" (restored after Duration real seconds). */
	void TriggerHitStop(float Duration = 0.1155f, float Dilation = 0.04f);
	/** Plays the short combat camera-kick shake at the given scale. */
	void CameraKick(float Scale);

	ECombatStyle CurrentStyle = ECombatStyle::Melee;

	/** Per-ability "ready again at" world time (cooldowns). */
	TMap<EActiveAbility, float> AbilityReadyTime;

	float LastAttackTime = -1000.f;
	double LastHitLandedTime = -1.0;

	bool bNextSwingLeft = true;    // toggles each melee attack for L/R alternation
	bool bLastSwingWasLeft = true; // side used by the in-flight swing; read by DoMeleeHit for flinch hint

	// Hit-stop bookkeeping (counted in real time so the dilation actually un-freezes).
	bool bHitStopActive = false;
	float HitStopRealLeft = 0.f;

	FTimerHandle MeleeHitTimer;
	/** Delays the mage spell bolt until the cast animation's release frame (see Attack/Mage). */
	FTimerHandle MageCastTimer;
};
