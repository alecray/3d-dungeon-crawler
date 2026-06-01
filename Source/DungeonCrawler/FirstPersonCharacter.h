#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstPersonCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UInputAction;
class UInputMappingContext;
class UHealthComponent;
struct FInputActionValue;

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

protected:
	virtual void BeginPlay() override;

	// ---- Camera ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// ---- Graybox hands (parented to the camera so they track the view) ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hands")
	TObjectPtr<UStaticMeshComponent> LeftHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hands")
	TObjectPtr<UStaticMeshComponent> RightHand;

	/** Holder for the sword, parented to the camera (not the hand cube, so the hand's small scale
	 *  doesn't shrink it). Both the graybox placeholder and the optional real mesh hang off this, and
	 *  it's what bobs/thrusts with the hands. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hands")
	TObjectPtr<USceneComponent> SwordRoot;

	/** Real-mesh sword. Used when SwordMeshAsset is set (defaults to /Game/Weapons/SM_Sword if it
	 *  exists); otherwise hidden in favor of the graybox placeholder below. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hands")
	TObjectPtr<UStaticMeshComponent> SwordMesh;

	/** Static mesh used for the sword. Assign your imported Blender sword here to replace the
	 *  graybox placeholder (no code change needed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands")
	TObjectPtr<UStaticMesh> SwordMeshAsset;

	/** Cube parts making up the graybox placeholder sword (blade/guard/grip/pommel). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> SwordPlaceholderParts;

	// ---- Torch (floats above the left hand; toggled with F, off by default) ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torch")
	TObjectPtr<UPointLightComponent> TorchLight;

	/** Small graybox cube standing in for the torch object itself. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torch")
	TObjectPtr<UStaticMeshComponent> TorchMesh;

	// ---- Health ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UHealthComponent> Health;

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
	TObjectPtr<UInputAction> ToggleTorchAction;

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

	// ---- Hand walking sway (head-bob style) ----
	/** Vertical bob amplitude of the hands while walking (cm). */
	UPROPERTY(EditAnywhere, Category = "Hands")
	float BobVertAmplitude = 1.2f;

	/** Side-to-side sway amplitude of the hands while walking (cm). */
	UPROPERTY(EditAnywhere, Category = "Hands")
	float BobSideAmplitude = 0.8f;

	/** Steps per second of bob at full walk speed. */
	UPROPERTY(EditAnywhere, Category = "Hands")
	float BobStepRate = 1.9f;

	/** How quickly the bob blends in/out as the player starts/stops moving. */
	UPROPERTY(EditAnywhere, Category = "Hands")
	float BobBlendSpeed = 8.f;

	// ---- Melee attack (fists / sword swing) ----
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

	/** How long the forward hand/sword thrust animation lasts (s). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackThrustDuration = 0.18f;

	/** How far the hands/sword punch forward during a swing (cm). */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackThrustDistance = 14.f;

private:
	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Attack(const FInputActionValue& Value);
	void ToggleTorch(const FInputActionValue& Value);

	void HandleDeath(UHealthComponent* DeadComponent);

	// Builds the Enhanced Input actions + mapping context once (called before bind/registration).
	void EnsureInputAssets();

	bool bInputAssetsBuilt = false;

	// Resting local offsets of the hands/sword relative to the camera (animations move around these).
	FVector LeftHandBase = FVector::ZeroVector;
	FVector RightHandBase = FVector::ZeroVector;
	FVector SwordBase = FVector::ZeroVector;
	FVector TorchBase = FVector::ZeroVector;

	bool bTorchOn = false;

	float BobPhase = 0.f;   // advancing sway phase, in radians
	float BobWeight = 0.f;  // 0 = idle (no sway), 1 = fully walking

	float LastAttackTime = -1000.f;
	float AttackThrustTimeLeft = 0.f; // counts down during the swing thrust
	bool bDead = false;
};
