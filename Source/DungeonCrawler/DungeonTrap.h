#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonTrap.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UStaticMesh;

/** The kinds of hazard the dungeon can scatter. */
UENUM(BlueprintType)
enum class ETrapType : uint8
{
	/** Floor spikes that rise and fall on a repeating timer — a timing hazard to run past. */
	SpikeFloor    UMETA(DisplayName = "Spike Floor"),
	/** Wall-mounted launcher that fires a dart down its facing on a timer. */
	DartShooter   UMETA(DisplayName = "Dart Shooter"),
	/** A floor plate that, when stepped on, erupts spikes once then re-arms. */
	PressurePlate UMETA(DisplayName = "Pressure Plate"),

	MAX           UMETA(Hidden)
};

/**
 * A self-contained dungeon hazard, assembled from engine cube primitives based on TrapType (graybox).
 * Each visible part has a mesh-override swap-in point (BaseMeshOverride / SpikeMeshOverride /
 * DartMeshOverride) — drop a finished FBX onto the matching property, or import to the conventional
 * /Game/Traps path, and the graybox cubes are replaced without touching the generator. The base of
 * the trap sits at Z = 0 (floor level); wall-mounted traps face +X (their local forward).
 */
UCLASS()
class DUNGEONCRAWLER_API ADungeonTrap : public AActor
{
	GENERATED_BODY()

public:
	ADungeonTrap();

	/** Sets the type and (re)builds the geometry. Call right after spawning. */
	void Configure(ETrapType InType);

	ETrapType GetTrapType() const { return TrapType; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Which hazard this actor represents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap")
	ETrapType TrapType = ETrapType::SpikeFloor;

	// ---- Finished-mesh swap-in points (null = build the graybox cubes for that part). ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Meshes")
	TObjectPtr<UStaticMesh> BaseMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Meshes")
	TObjectPtr<UStaticMesh> SpikeMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Meshes")
	TObjectPtr<UStaticMesh> DartMeshOverride;

	// ---- Tuning ----
	/** Damage dealt per spike hit, per dart hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Tuning", meta = (ClampMin = "0"))
	float Damage = 25.f;

	/** Minimum seconds between damage ticks to a victim standing in a spike hazard. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0.05"))
	float DamageInterval = 0.6f;

	/** How long the spikes stay fully raised. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0"))
	float SpikeUpTime = 1.1f;

	/** (SpikeFloor) How long the spikes stay retracted between cycles. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0"))
	float SpikeDownTime = 2.0f;

	/** Seconds for the spikes to travel up or down. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0.02"))
	float SpikeMoveTime = 0.12f;

	/** (PressurePlate) Warning delay between stepping on the plate and the spikes erupting. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0"))
	float PlateTelegraph = 0.45f;

	/** (PressurePlate) Seconds the plate stays spent before re-arming. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0"))
	float PlateRearmTime = 2.5f;

	/** (DartShooter) Seconds between dart shots. */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "0.1"))
	float DartInterval = 2.4f;

	/** (DartShooter) Maximum dart travel distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Trap|Tuning", meta = (ClampMin = "100"))
	float DartRange = 1600.f;

private:
	// ---- Build ----
	void Rebuild();
	void BuildSpikeTrap();   // shared by SpikeFloor and PressurePlate
	void BuildDartShooter();
	UStaticMeshComponent* AddBox(USceneComponent* Parent, const FVector& Center, const FVector& SizeCm, bool bCollide);
	/** Returns Override if set, else the mesh at SoftPath if it exists, else null (build graybox). */
	UStaticMesh* ResolveMesh(UStaticMesh* Override, const TCHAR* SoftPath) const;

	// ---- Behaviour ----
	void TickSpikeFloor(float Dt);
	void TickPressurePlate(float Dt);
	void TickDartShooter(float Dt);
	void TickTracers(float Dt);
	/** Applies damage to any pawn currently inside the hazard footprint (on the DamageInterval cadence). */
	void DamageOverlappers();
	void FireDart();
	/** Moves the spike group so its tips sit Alpha of the way out of the floor (0 = hidden, 1 = full). */
	void SetSpikeExtension(float Alpha);
	void SetPlateDepressed(bool bDown);

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<USceneComponent> SpikeGroup; // raised/lowered as a unit
	UPROPERTY() TObjectPtr<USceneComponent> Plate;      // pressure plate that depresses
	UPROPERTY() TObjectPtr<USceneComponent> Muzzle;     // dart spawn point
	UPROPERTY() TObjectPtr<UBoxComponent> Trigger;      // hazard footprint / step volume
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;

	// Transient dart tracers (graybox streaks); components are owned by this actor so GC-safe.
	struct FTracer { UStaticMeshComponent* Comp = nullptr; float Life = 0.f; };
	TArray<FTracer> Tracers;

	/** Hazard state machine: meaning depends on TrapType (see the Tick* handlers). */
	enum class EState : uint8 { Idle, Telegraph, Rising, Raised, Falling, Cooldown };
	EState State = EState::Idle;
	float StateTimer = 0.f;    // seconds elapsed in the current state
	float DamageTimer = 0.f;   // counts down to the next allowed damage tick
	float FireTimer = 0.f;     // counts down to the next dart shot
};
