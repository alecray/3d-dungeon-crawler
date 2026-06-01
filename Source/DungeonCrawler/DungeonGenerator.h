#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class AMonsterCharacter;
class ABossMonster;

/** A placed room as a rectangle of grid cells. */
USTRUCT()
struct FDungeonRoom
{
	GENERATED_BODY()

	int32 X = 0; // min cell X
	int32 Y = 0; // min cell Y
	int32 W = 0; // width in cells
	int32 H = 0; // height in cells

	int32 CenterX() const { return X + W / 2; }
	int32 CenterY() const { return Y + H / 2; }
};

/**
 * Procedural graybox dungeon. A grid/tilemap is carved into rooms (multi-cell rectangles) joined by
 * 1-cell-wide L-shaped corridors, then geometry is built: every floor cell gets a floor + ceiling
 * tile, and a wall is raised on every cell edge that borders a non-floor cell — so doorways appear
 * automatically where corridors meet rooms. Floor/wall/ceiling tiles are instanced (one draw path
 * each) for performance; furniture props and per-room point lights are spawned as actors.
 */
UCLASS()
class DUNGEONCRAWLER_API ADungeonGenerator : public AActor
{
	GENERATED_BODY()

public:
	ADungeonGenerator();

	/** Wipes any existing layout and builds a fresh dungeon. Safe to call again to regenerate. */
	UFUNCTION(CallInEditor, Category = "Dungeon")
	void Generate();

	/** World-space center (at floor level, Z = 0) of room #Index, or actor location if none. */
	FVector GetRoomCenterWorld(int32 Index) const;

	int32 GetRoomCount() const { return Rooms.Num(); }

protected:
	virtual void BeginPlay() override;

	// ---- Grid / layout tunables ----
	UPROPERTY(EditAnywhere, Category = "Dungeon|Grid", meta = (ClampMin = "8"))
	int32 GridWidth = 48;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Grid", meta = (ClampMin = "8"))
	int32 GridHeight = 48;

	/** Size of one grid cell in cm (also the hallway width). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Grid", meta = (ClampMin = "100"))
	float CellSize = 300.f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Rooms", meta = (ClampMin = "1"))
	int32 TargetRoomCount = 8;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Rooms", meta = (ClampMin = "2"))
	int32 MinRoomCells = 3;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Rooms", meta = (ClampMin = "2"))
	int32 MaxRoomCells = 7;

	/** How hard to try placing each room before giving up. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Rooms", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 200;

	// ---- Geometry tunables ----
	UPROPERTY(EditAnywhere, Category = "Dungeon|Geometry", meta = (ClampMin = "100"))
	float WallHeight = 320.f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Geometry", meta = (ClampMin = "2"))
	float WallThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Geometry", meta = (ClampMin = "2"))
	float SlabThickness = 20.f;

	// ---- Props ----
	/** Chance [0..1] that any given interior room cell receives a furniture prop. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Props", meta = (ClampMin = "0", ClampMax = "1"))
	float PropFillChance = 0.14f;

	// ---- Monsters ----
	/** Monster class to spawn in groups (defaults to AMonsterCharacter). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Monsters")
	TSubclassOf<AMonsterCharacter> MonsterClass;

	/** Chance [0..1] that a room (other than the player's start room) hosts a monster group. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Monsters", meta = (ClampMin = "0", ClampMax = "1"))
	float MonsterGroupChance = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Monsters", meta = (ClampMin = "1"))
	int32 MinGroupSize = 2;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Monsters", meta = (ClampMin = "1"))
	int32 MaxGroupSize = 4;

	// ---- Boss ----
	/** Boss class spawned in the boss room (defaults to ABossMonster). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Boss")
	TSubclassOf<ABossMonster> BossClass;

	/** Side length (in cells) of the square boss room — much larger than normal rooms. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Boss", meta = (ClampMin = "6"))
	int32 BossRoomCells = 13;

	// ---- Lighting (wall torches) ----
	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting")
	bool bSpawnTorches = true;

	/** Place a wall torch roughly every N cells along walls (rooms and hallways). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "1"))
	int32 TorchSpacingCells = 3;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "0"))
	float TorchLightIntensity = 2400.f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "0"))
	float TorchLightRadius = 950.f;

	// ---- Determinism ----
	/** Seed for layout. < 0 picks a fresh random seed every Generate(). */
	UPROPERTY(EditAnywhere, Category = "Dungeon")
	int32 RandomSeed = -1;

private:
	enum class ECell : uint8 { Empty, Room, Corridor };

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FloorISM;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> CeilingISM;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WallISM;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> TorchISM;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TArray<FDungeonRoom> Rooms;

	/** Index into Rooms of the big boss room (the furthest room from the start), or INDEX_NONE. */
	int32 BossRoomIndex = INDEX_NONE;

	/** Props and lights spawned for the current layout, tracked so a regenerate can clear them. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TArray<ECell> Cells;
	FRandomStream Rng;

	// Grid helpers
	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < GridWidth && Y < GridHeight; }
	int32 Idx(int32 X, int32 Y) const { return Y * GridWidth + X; }
	ECell CellAt(int32 X, int32 Y) const { return InBounds(X, Y) ? Cells[Idx(X, Y)] : ECell::Empty; }
	bool IsFloor(int32 X, int32 Y) const { return CellAt(X, Y) != ECell::Empty; }
	FVector CellToLocal(int32 X, int32 Y) const;

	// Pipeline steps
	void ClearLayout();
	void PlaceRooms();
	void PlaceBossRoom();
	void CarveCorridors();
	void CarveLine(int32 X0, int32 Y0, int32 X1, int32 Y1, bool bHorizontalFirst);
	void BuildGeometry();
	void AddTile(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& SizeCm);
	void ScatterProps();
	void ScatterMonsters();
	void SpawnBoss();
	void SpawnWallTorches();
	/** Places one wall torch (bracket mesh + warm light) on the wall in the given outward direction. */
	void PlaceWallTorch(const FVector& CellLocal, const FVector& OutwardDir);
};
