#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class AMonsterCharacter;
class ABossMonster;
class ALootChest;
class APortal;
class ADungeonTrap;
class ABossArena;
class UHealthComponent;

/** Special-purpose tag for a room, driving its monsters / loot / decoration. */
enum class ERoomType : uint8
{
	Normal,   // standard: chance-based monster group + loot
	Treasure, // guaranteed extra chests, guarded by a group
	Ambush,   // a larger forced swarm, no loot
	Rest,     // safe breather: no monsters, no traps
	Elite,    // a single tough elite + a guaranteed chest
};

/** A placed room as a rectangle of grid cells. */
USTRUCT()
struct FDungeonRoom
{
	GENERATED_BODY()

	int32 X = 0; // min cell X
	int32 Y = 0; // min cell Y
	int32 W = 0; // width in cells
	int32 H = 0; // height in cells

	ERoomType Type = ERoomType::Normal;

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

	/** True once a boss room has been placed. */
	bool HasBossRoom() const { return BossRoomIndex != INDEX_NONE; }
	/** World-space center of the boss room (floor level), or actor location if none. */
	FVector GetBossRoomCenterWorld() const { return GetRoomCenterWorld(BossRoomIndex); }

	// ---- Minimap read API ----
	int32 GetGridWidth() const { return GridWidth; }
	int32 GetGridHeight() const { return GridHeight; }
	/** True if cell (X,Y) is walkable floor (room or corridor). */
	bool IsFloorCell(int32 X, int32 Y) const { return IsFloor(X, Y); }
	/** True if cell (X,Y) lies inside the boss room. */
	bool IsBossRoomCell(int32 X, int32 Y) const;
	/** Maps a world position to its grid cell; returns false if outside the grid. */
	bool WorldToCell(const FVector& World, int32& OutX, int32& OutY) const;

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
	int32 MinRoomCells = 4;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Rooms", meta = (ClampMin = "2"))
	int32 MaxRoomCells = 9;

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
	/** Chance [0..1] that any given interior room cell receives a lone (scattered) furniture prop. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Props", meta = (ClampMin = "0", ClampMax = "1"))
	float PropFillChance = 0.14f;

	/** Chance [0..1] that any given corridor cell receives a wall-hugging prop. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Props", meta = (ClampMin = "0", ClampMax = "1"))
	float CorridorPropChance = 0.18f;

	/** Furniture clusters (piles) per interior room cell — more = more clumps scattered around. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Props", meta = (ClampMin = "0"))
	float PropClusterDensity = 0.14f;

	/** Number of props piled into each cluster. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Props", meta = (ClampMin = "1"))
	int32 ClusterPropsMin = 3;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Props", meta = (ClampMin = "1"))
	int32 ClusterPropsMax = 6;

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

	/** Boss encounter manager (trigger + spawn-on-entry + door seal + intro). Defaults to ABossArena. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Boss")
	TSubclassOf<ABossArena> ArenaClass;

	/** Side length (in cells) of the square boss room — much larger than normal rooms. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Boss", meta = (ClampMin = "6"))
	int32 BossRoomCells = 13;

	// ---- Loot ----
	/** Loot chest class (defaults to ALootChest). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Loot")
	TSubclassOf<ALootChest> ChestClass;

	/** Chance [0..1] that a (non-start) room contains a loot chest. The boss room always gets one. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float ChestChancePerRoom = 0.55f;

	// ---- Traps ----
	/** Trap class scattered as dungeon hazards (defaults to ADungeonTrap). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Traps")
	TSubclassOf<ADungeonTrap> TrapClass;

	/** Chance [0..1] that a corridor cell hosts a floor trap (spike floor or pressure plate). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Traps", meta = (ClampMin = "0", ClampMax = "1"))
	float CorridorTrapChance = 0.08f;

	/** Chance [0..1] that an interior room cell hosts a floor trap. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Traps", meta = (ClampMin = "0", ClampMax = "1"))
	float RoomTrapChance = 0.02f;

	/** Chance [0..1] that a wall cell beside open floor mounts a dart shooter. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Traps", meta = (ClampMin = "0", ClampMax = "1"))
	float DartShooterChance = 0.025f;

	// ---- Return portals (back to town) ----
	/** Portal class for the return-to-town portals (defaults to APortal). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Portals")
	TSubclassOf<APortal> PortalClass;

	/** Level name the return portals travel to. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Portals")
	FName TownMapName = TEXT("L_Town");

	// ---- Lighting (wall torches) ----
	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting")
	bool bSpawnTorches = true;

	/** Place a wall torch roughly every N cells along walls (rooms and hallways). */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "1"))
	int32 TorchSpacingCells = 3;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "0"))
	float TorchLightIntensity = 2600.f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "0"))
	float TorchLightRadius = 900.f;

	/** Target height (cm) the torch mesh is scaled to, so it reads at a sensible size regardless of
	 *  the imported mesh's native scale. */
	UPROPERTY(EditAnywhere, Category = "Dungeon|Lighting", meta = (ClampMin = "1"))
	float TorchMeshHeight = 70.f;

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

	/** Imported wall-torch mesh (falls back to graybox cubes if absent). */
	UPROPERTY()
	TObjectPtr<UStaticMesh> TorchMesh;

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
	/** Tags non-start, non-boss rooms with a special type (treasure/ambush/rest/elite). */
	void AssignRoomTypes();
	/** Spawns a colored marker light in each special room so its type reads at a glance. */
	void DecorateRooms();
	void CarveCorridors();
	void CarveLine(int32 X0, int32 Y0, int32 X1, int32 Y1, bool bHorizontalFirst);
	void BuildGeometry();
	void AddTile(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& SizeCm);
	void ScatterProps();
	void ScatterMonsters();
	void ScatterChests();
	void ScatterTraps();
	/** Spawns + configures the boss-room encounter manager (boss spawns when the player enters). */
	void SetupBossEncounter();
	/** Spawns the always-on return portal in the start room (the boss-room one is spawned by the arena). */
	void SpawnReturnPortals();
	void SpawnWallTorches();
	/** Places one wall torch (bracket mesh + warm light) on the wall in the given outward direction. */
	void PlaceWallTorch(const FVector& CellLocal, const FVector& OutwardDir);
};
