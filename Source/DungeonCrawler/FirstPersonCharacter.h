#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SkillTypes.h" // EActiveAbility
#include "FirstPersonCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class UInputAction;
class UInputMappingContext;
class UHealthComponent;
class UResourceComponent;
class UStatsComponent;
class UInventoryComponent;
class UHotbarComponent;
class USkillTreeComponent;
class UEquipmentComponent;
class USkeletalMesh;
class UAnimSequence;
class AProjectile;
struct FInputActionValue;
struct FInputActionInstance;
enum class ECharacterClass : uint8; // defined in CharacterClass.h

/** How the equipped weapon attacks (ranged/mage behaviour arrives in Phase 3). */
UENUM(BlueprintType)
enum class ECombatStyle : uint8
{
	Melee,
	Ranged,
	Mage
};

/**
 * Player-controlled first-person character: an eye-height camera that follows the controller
 * rotation, two graybox cube "hands" parented to the camera, and WASD + mouse-look movement
 * driven by Enhanced Input that is configured entirely in C++ (no input assets).
 */
UCLASS()
class DUNGEONCRAWLER_API AFirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFirstPersonCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	// Accessors for the HUD and other systems.
	UHealthComponent* GetHealthComponent() const { return Health; }
	UResourceComponent* GetManaComponent() const { return Mana; }
	UResourceComponent* GetStaminaComponent() const { return Stamina; }
	UStatsComponent* GetStatsComponent() const { return Stats; }
	UInventoryComponent* GetInventoryComponent() const { return Inventory; }
	UHotbarComponent* GetHotbarComponent() const { return Hotbar; }
	USkillTreeComponent* GetSkillTreeComponent() const { return SkillTree; }
	UEquipmentComponent* GetEquipmentComponent() const { return Equipment; }

	/** Verb for the interactable currently under the crosshair (e.g. "Open"), or empty if none. */
	FString GetInteractionPrompt() const;

	/** Set while the player is seated at a blackjack table; suppresses the world interaction prompt. */
	void SetBlackjackActive(bool bActive) { bBlackjackActive = bActive; }

	// ---- Insufficient-resource feedback (HUD polls these to flash the bar red) ----
	/** Call when an action is denied for lack of stamina / mana — flashes the matching bar. */
	void FlagStaminaDenied();
	void FlagManaDenied();
	/** 0 = no flash, 1 = just denied (decays). Read by the HUD to tint the bar. */
	float GetStaminaDenyFlash() const { return StaminaDenyFlashLeft / FMath::Max(0.01f, ResourceDenyFlashDuration); }
	float GetManaDenyFlash() const { return ManaDenyFlashLeft / FMath::Max(0.01f, ResourceDenyFlashDuration); }

	// ---- Gold (shop) ----
	int32 GetGold() const { return Gold; }
	void AddGold(int32 Amount);          // adds (clamped >= 0) and saves
	bool TrySpendGold(int32 Amount);     // false if insufficient; otherwise deducts and saves

	// ---- Dev tools (invoked from the pause menu's Dev panel) ----
	bool DevToggleNoClip();   // fly through walls; returns the new on/off state
	bool DevToggleGodMode();  // invulnerability; returns the new on/off state
	void DevKill();           // force player death

	/** Captures and writes the player profile to disk (e.g. before traveling between levels). */
	void SaveNow() { PersistProfile(); }

	/** Applies a starting class's attribute spread + weapon to a fresh character (from the class-select menu). */
	void ApplyClassLoadout(ECharacterClass Class);

	// ---- Settings (pause menu) ----
	float GetLookSensitivity() const { return LookSensitivity; }
	void SetLookSensitivity(float V) { LookSensitivity = FMath::Clamp(V, 0.1f, 4.f); }

protected:
	virtual void BeginPlay() override;

	// ---- Camera ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Skeletal-mesh sword held in view (parented to the camera). A skeletal mesh is used so it can
	 *  play Blender-authored swing animations. Defaults to /Game/Weapons/Sword/SK_Sword. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<USkeletalMeshComponent> SwordMesh;

	/** Skeletal mesh for the sword. Assign your imported sword here to override the default path. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<USkeletalMesh> SwordSkeletalAsset;

	/** Swing animation played on attack. Defaults to /Game/Weapons/Sword/A_Sword_Attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<UAnimSequence> SwingAnim;

	/** Deflect/bounce animation played when a melee swing strikes a solid, non-damageable surface (wall,
	 *  prop) instead of an enemy. Defaults to /Game/Weapons/Sword/A_Sword_Deflect (make this anim). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<UAnimSequence> DeflectAnim;

	/** Optional crossbow skeletal mesh (equipping the Crossbow item swaps to it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<USkeletalMesh> CrossbowSkeletalAsset;

	/** Crossbow fire animation. Defaults to /Game/Weapons/Crossbow/A_Crossbow_Shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<UAnimSequence> CrossbowShootAnim;

	/** Optional staff/wand skeletal mesh (equipping a Staff item swaps to it; null = no held mesh yet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<USkeletalMesh> StaffSkeletalAsset;

	// ---- Ranged / mage ----
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MeleeStaminaCost = 10.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float RangedStaminaCost = 12.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float SpellManaCost = 18.f;

	/** Base projectile damage (scaled by Dexterity for ranged / Intelligence for mage). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileDamage = 18.f;

	// ---- Stats & resources ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UStatsComponent> Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	TObjectPtr<UResourceComponent> Mana;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	TObjectPtr<UResourceComponent> Stamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UHotbarComponent> Hotbar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<USkillTreeComponent> SkillTree;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UEquipmentComponent> Equipment;

	// ---- Enhanced Input (created & configured in C++, no assets) ----
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY()
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY()
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InventoryToggleAction;

	UPROPERTY()
	TObjectPtr<UInputAction> CollectionToggleAction;

	UPROPERTY()
	TObjectPtr<UInputAction> SkillTreeToggleAction;

	UPROPERTY()
	TObjectPtr<UInputAction> AbilityAction;

	/** Number keys 1-8 select hotbar slots. */
	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> HotbarActions;

	/** Reach of the interact line-trace (cm). */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractRange = 280.f;

	// ---- Tunables ----
	/** Mouse look sensitivity multiplier. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float LookSensitivity = 1.0f;

	/** Camera pitch clamp (degrees): how far down the view may look. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPitchMin = -80.f;

	/** Camera pitch clamp (degrees): how far up the view may look. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPitchMax = 80.f;

	// ---- Sword walking sway (head-bob style) ----
	/** Vertical bob amplitude of the sword while walking (cm). */
	UPROPERTY(EditAnywhere, Category = "Sword")
	float BobVertAmplitude = 1.2f;

	/** Side-to-side sway amplitude of the sword while walking (cm). */
	UPROPERTY(EditAnywhere, Category = "Sword")
	float BobSideAmplitude = 0.8f;

	/** Steps per second of bob at full walk speed. */
	UPROPERTY(EditAnywhere, Category = "Sword")
	float BobStepRate = 1.9f;

	/** How quickly the bob blends in/out as the player starts/stops moving. */
	UPROPERTY(EditAnywhere, Category = "Sword")
	float BobBlendSpeed = 8.f;

	// ---- Melee attack (sword swing) ----
	/** Reach of a melee swing from the camera (cm). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 220.f;

	/** Radius of the swing's sphere sweep (cm). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRadius = 45.f;

	/** Damage dealt per landed swing. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 22.f;

	/** Minimum seconds between swings. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackCooldown = 0.45f;

	// ---- Movement / dash ----
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 450.f;

	/** Speed during the dash burst (Shift). A short, committed lurch — a Dark-Souls-style dodge. Tuned so
	    the dash (~400cm) clears the boss's danger zone while a plain walk-back during the wind-up can't. */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float DashSpeed = 2000.f;

	/** How long the dash burst lasts (s). Distance ≈ DashSpeed * DashDuration. */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float DashDuration = 0.2f;

	/** Minimum gap between dashes (s). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float DashCooldown = 0.55f;

	/** Stamina spent per dash. */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float DashStaminaCost = 25.f;

private:
	bool bBlackjackActive = false; // true while seated at a blackjack table — hides the interact prompt

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Attack(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void ToggleInventory(const FInputActionValue& Value);
	void ToggleCollectionLog(const FInputActionValue& Value);
	void ToggleSkillTree(const FInputActionValue& Value);

	/** Q: use the active ability for the current combat style (if unlocked + off cooldown + affordable). */
	void UseAbility(const FInputActionValue& Value);
	/** Damages every monster within Radius of the player (Whirlwind / Nova). */
	void PerformAreaBurst(float Radius, float Damage);

	/** Per-ability "ready again at" world time (cooldowns). */
	TMap<EActiveAbility, float> AbilityReadyTime;

	void HandleDeath(UHealthComponent* DeadComponent);
	void HandleStatsChanged(UStatsComponent* ChangedStats);
	void HandleInventoryChanged(UInventoryComponent* ChangedInventory);
	void HandleItemDiscovered(FName ItemId);
	void HandleHotbarChanged(UHotbarComponent* ChangedHotbar);
	void OnHotbarKey(const FInputActionInstance& Instance);

	/** Swap the held weapon mesh + combat style to match the active hotbar slot. */
	void EquipActiveHotbarItem();

	void MeleeAttack();
	/** The actual swing hit (sweep + damage), fired on a timer partway through the swing animation. */
	void DoMeleeHit();
	/** Plays the deflect/bounce reaction when a swing hits a solid non-damageable surface. */
	void PlayMeleeDeflect(const FVector& ImpactPoint);
	FTimerHandle MeleeHitTimer;

	/** Briefly slows global time for impact "hit-stop" (restored after Duration real seconds). */
	void TriggerHitStop(float Duration = 0.165f, float Dilation = 0.04f); // ~3x (was 0.055) for a punchier hit
	/** Plays the short combat camera-kick shake at the given scale. */
	void CameraKick(float Scale);

	// Hit-stop bookkeeping (counted in real time so the dilation actually un-freezes).
	bool bHitStopActive = false;
	float HitStopRealLeft = 0.f;
	/** Fires the center bolt plus ExtraProjectiles additional bolts in a horizontal spread. */
	void FireProjectile(float Damage, int32 ExtraProjectiles = 0);

	/** Push current stats into the GameInstance profile and write it to disk. */
	void PersistProfile();

	/** Recompute resource maxes from the stats component. */
	void RefreshResourceMaxes(bool bRefill);

	// Builds the Enhanced Input actions + mapping context once (called before bind/registration).
	void EnsureInputAssets();

	bool bInputAssetsBuilt = false;

	// Resting local offset of the sword relative to the camera (the bob animates around this).
	FVector SwordBase = FVector::ZeroVector;

	float BobPhase = 0.f;   // advancing sway phase, in radians
	float BobWeight = 0.f;  // 0 = idle (no sway), 1 = fully walking

	/** Distance walked since the last footstep puff; a small dust burst spawns every FootstepInterval cm. */
	float StepAccum = 0.f;
	UPROPERTY(EditAnywhere, Category = "Feel") float FootstepInterval = 175.f;

	float LastAttackTime = -1000.f;
	bool bDead = false;
	bool bNoClip = false;

	// ---- Insufficient-resource feedback ----
	UPROPERTY(EditAnywhere, Category = "Feel") float ResourceDenyFlashDuration = 0.4f;
	float StaminaDenyFlashLeft = 0.f;
	float ManaDenyFlashLeft = 0.f;

	// ---- Dash state ----
	FVector DashDir = FVector::ZeroVector; // locked-in direction of the current dash
	float DashTimeLeft = 0.f;              // remaining dash-burst time
	float DashCooldownLeft = 0.f;          // remaining cooldown before the next dash
	float DefaultGroundFriction = 8.f;     // captured in BeginPlay; restored after the dash glide
	float DefaultBrakingWalk = 2048.f;

	// ---- Game-feel: dash FOV kick + low-HP screen effect ----
	UPROPERTY(EditAnywhere, Category = "Feel") float SprintFOVKick = 8.f;
	UPROPERTY(EditAnywhere, Category = "Feel") float LowHpThreshold = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Feel") float LowHpFringe = 4.f;   // chromatic aberration at 0 HP
	UPROPERTY(EditAnywhere, Category = "Feel") float LowHpDesat = 0.5f;   // desaturation at 0 HP
	float BaseFOV = 90.f;
	int32 LastLevel = 1;  // to detect level-ups for the celebration burst
	ECombatStyle CurrentStyle = ECombatStyle::Melee;

	/** Player gold (persisted in the profile). */
	int32 Gold = 0;
};
