#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
class USkeletalMesh;
struct FInputActionValue;
struct FInputActionInstance;

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

	/** Swing animation played on attack. Defaults to /Game/Weapons/Sword/A_Sword_Swing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<UAnimSequence> SwingAnim;

	/** Optional crossbow skeletal mesh (equipping the Crossbow item swaps to it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<USkeletalMesh> CrossbowSkeletalAsset;

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
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InventoryToggleAction;

	UPROPERTY()
	TObjectPtr<UInputAction> CollectionToggleAction;

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

	// ---- Movement / sprint ----
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintSpeed = 720.f;

	/** Stamina drained per second while sprinting. */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintStaminaPerSecond = 24.f;

private:
	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Attack(const FInputActionValue& Value);
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void ToggleInventory(const FInputActionValue& Value);
	void ToggleCollectionLog(const FInputActionValue& Value);

	void HandleDeath(UHealthComponent* DeadComponent);
	void HandleStatsChanged(UStatsComponent* ChangedStats);
	void HandleInventoryChanged(UInventoryComponent* ChangedInventory);
	void HandleItemDiscovered(FName ItemId);
	void HandleHotbarChanged(UHotbarComponent* ChangedHotbar);
	void OnHotbarKey(const FInputActionInstance& Instance);

	/** Swap the held weapon mesh + combat style to match the active hotbar slot. */
	void EquipActiveHotbarItem();

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

	float LastAttackTime = -1000.f;
	bool bDead = false;
	bool bSprintHeld = false;
	ECombatStyle CurrentStyle = ECombatStyle::Melee;

	/** Player gold (persisted in the profile). */
	int32 Gold = 0;
};
