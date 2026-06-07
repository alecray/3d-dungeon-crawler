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
class UFishingComponent;
class USkeletalMesh;
class UAnimSequence;
class AProjectile;
class UCombatComponent;
enum class EFishingPose : uint8; // defined in FishingComponent.h
enum class ECombatStyle : uint8; // defined in CombatComponent.h
struct FInputActionValue;
struct FInputActionInstance;
enum class ECharacterClass : uint8; // defined in CharacterClass.h

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
	UFishingComponent* GetFishingComponent() const { return Fishing; }
	UCombatComponent* GetCombatComponent() const { return Combat; }

	/** The in-view weapon mesh (sword/crossbow/staff, or the fishing rod while fishing). Used by the
	 *  fishing + combat components to swap meshes / play animations. */
	USkeletalMeshComponent* GetHeldWeaponMesh() const { return SwordMesh; }

	/** The first-person camera, used by UCombatComponent for attack traces and projectile spawns. */
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	/** True once the player has died (the death flow has started). */
	bool IsDead() const { return bDead; }

	/** Swap the held weapon mesh + combat style to match the active hotbar slot (also used to restore the
	 *  weapon after fishing). */
	void EquipActiveHotbarItem();

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

	/** World time the player last landed a damaging hit; the HUD polls this to flash the hit marker.
	 *  Forwards to the combat component. */
	double GetLastHitLandedTime() const;

	// ---- Gold (shop) — owned by the GameInstance profile; these forward to it then persist. ----
	int32 GetGold() const;
	void AddGold(int32 Amount);          // adds (clamped >= 0) and saves
	bool TrySpendGold(int32 Amount);     // false if insufficient; otherwise deducts and saves

	// ---- No-clip (dev fly-through; toggled by the controller's dev menu, read by Dash/footsteps) ----
	void SetNoClip(bool bEnabled);
	bool IsNoClip() const { return bNoClip; }

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

	/** Skeletal mesh for the sword. Soft ref defaulted to the conventional path; assign a different asset
	 *  here to override. Force-loaded in BeginPlay, then read via .Get() at the use sites. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TSoftObjectPtr<USkeletalMesh> SwordSkeletalAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Weapons/Sword/SK_Sword.SK_Sword")));

	/** Optional crossbow skeletal mesh (equipping the Crossbow item swaps to it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TSoftObjectPtr<USkeletalMesh> CrossbowSkeletalAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Weapons/Crossbow/SK_Crossbow.SK_Crossbow")));

	/** Optional staff/wand skeletal mesh (equipping a Staff item swaps to it; null = no held mesh yet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TSoftObjectPtr<USkeletalMesh> StaffSkeletalAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Weapons/Staff/SK_Staff.SK_Staff")));

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

	/** Fishing state/behaviour (rod swap, poses, status line); driven by AFishingHole. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fishing")
	TObjectPtr<UFishingComponent> Fishing;

	/** Attack execution (melee/ranged/mage), abilities, hit-stop + camera kick. Bound to the Attack/Q inputs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCombatComponent> Combat;

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

	/** Invincibility window at the START of a dash (s) — the Souls-style dodge "roll-through": time the dash
	    into an attack and you take no damage. 0 disables i-frames (pure positional dodge). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float DashIFrameDuration = 0.15f;

private:
	bool bBlackjackActive = false; // true while seated at a blackjack table — hides the interact prompt

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void ToggleInventory(const FInputActionValue& Value);
	void ToggleCollectionLog(const FInputActionValue& Value);
	void ToggleSkillTree(const FInputActionValue& Value);

	void HandleDeath(UHealthComponent* DeadComponent);
	void HandleStatsChanged(UStatsComponent* ChangedStats);
	void HandleInventoryChanged(UInventoryComponent* ChangedInventory);
	void HandleItemDiscovered(FName ItemId);
	void HandleHotbarChanged(UHotbarComponent* ChangedHotbar);
	void OnHotbarKey(const FInputActionInstance& Instance);

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
	float DashIFrameLeft = 0.f;            // remaining dash invulnerability time
	bool bDashInvuln = false;              // true while WE hold the dash i-frame invulnerability (so we only clear our own)

	// ---- Game-feel: dash FOV kick + low-HP screen effect ----
	UPROPERTY(EditAnywhere, Category = "Feel") float SprintFOVKick = 8.f;
	UPROPERTY(EditAnywhere, Category = "Feel") float LowHpThreshold = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Feel") float LowHpFringe = 4.f;   // chromatic aberration at 0 HP
	UPROPERTY(EditAnywhere, Category = "Feel") float LowHpDesat = 0.5f;   // desaturation at 0 HP
	float BaseFOV = 90.f;
	int32 LastLevel = 1;  // to detect level-ups for the celebration burst
};
