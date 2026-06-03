#include "DungeonGenerator.h"
#include "DungeonProp.h"
#include "MonsterCharacter.h"
#include "MonsterTypes.h"
#include "BossMonster.h"
#include "LootChest.h"
#include "ItemPickup.h"
#include "Portal.h"
#include "HealthComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

// Optional imported environment meshes; fall back to the graybox cube when absent. (Add collision to
// these meshes themselves in the editor so they block the player.)
static const TCHAR* FloorMeshPath = TEXT("/Game/Furniture/SM_Floor.SM_Floor");
static const TCHAR* WallMeshPath = TEXT("/Game/Furniture/SM_Wall.SM_Wall");
static const TCHAR* TorchMeshPath = TEXT("/Game/Furniture/SM_Torch.SM_Torch");

// The engine cube is a 100cm cube centered on its origin, so an instance scale of 1.0 == 100cm.
static constexpr float GenUnitCm = 100.f;

ADungeonGenerator::ADungeonGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TorchFinder(TorchMeshPath);
	if (TorchFinder.Succeeded())
	{
		TorchMesh = TorchFinder.Object;
	}

	auto MakeISM = [&](const TCHAR* Name, bool bCollide) -> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetupAttachment(SceneRoot);
		ISM->SetMobility(EComponentMobility::Movable);
		if (CubeMesh)
		{
			ISM->SetStaticMesh(CubeMesh);
		}
		if (bCollide)
		{
			ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ISM->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return ISM;
	};

	FloorISM = MakeISM(TEXT("FloorISM"), /*bCollide*/ true);
	WallISM = MakeISM(TEXT("WallISM"), /*bCollide*/ true);
	CeilingISM = MakeISM(TEXT("CeilingISM"), /*bCollide*/ false);
	TorchISM = MakeISM(TEXT("TorchISM"), /*bCollide*/ false);
	if (TorchISM && TorchMesh)
	{
		TorchISM->SetStaticMesh(TorchMesh); // imported torch instead of the graybox cubes
	}

	MonsterClass = AMonsterCharacter::StaticClass();
	BossClass = ABossMonster::StaticClass();
	ChestClass = ALootChest::StaticClass();
}

void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();
	Generate();
}

FVector ADungeonGenerator::CellToLocal(int32 X, int32 Y) const
{
	// Center the whole grid on the actor origin.
	const float OffsetX = -(GridWidth - 1) * 0.5f * CellSize;
	const float OffsetY = -(GridHeight - 1) * 0.5f * CellSize;
	return FVector(X * CellSize + OffsetX, Y * CellSize + OffsetY, 0.f);
}

bool ADungeonGenerator::IsBossRoomCell(int32 X, int32 Y) const
{
	if (!Rooms.IsValidIndex(BossRoomIndex))
	{
		return false;
	}
	const FDungeonRoom& R = Rooms[BossRoomIndex];
	return X >= R.X && X < R.X + R.W && Y >= R.Y && Y < R.Y + R.H;
}

bool ADungeonGenerator::WorldToCell(const FVector& World, int32& OutX, int32& OutY) const
{
	// Inverse of CellToLocal: undo the actor transform, then the grid centering offset.
	const FVector Local = GetActorTransform().InverseTransformPosition(World);
	const float OffsetX = -(GridWidth - 1) * 0.5f * CellSize;
	const float OffsetY = -(GridHeight - 1) * 0.5f * CellSize;
	OutX = FMath::RoundToInt((Local.X - OffsetX) / CellSize);
	OutY = FMath::RoundToInt((Local.Y - OffsetY) / CellSize);
	return InBounds(OutX, OutY);
}

FVector ADungeonGenerator::GetRoomCenterWorld(int32 Index) const
{
	if (!Rooms.IsValidIndex(Index))
	{
		return GetActorLocation();
	}
	const FDungeonRoom& R = Rooms[Index];
	return GetActorTransform().TransformPosition(CellToLocal(R.CenterX(), R.CenterY()));
}

void ADungeonGenerator::Generate()
{
	if (RandomSeed < 0)
	{
		Rng.Initialize(FMath::Rand());
	}
	else
	{
		Rng.Initialize(RandomSeed);
	}

	ClearLayout();
	PlaceRooms();
	PlaceBossRoom();
	CarveCorridors();
	BuildGeometry();
	ScatterProps();
	ScatterMonsters();
	ScatterChests();
	SpawnBoss();
	SpawnReturnPortals();
	if (bSpawnTorches)
	{
		SpawnWallTorches();
	}
}

void ADungeonGenerator::ClearLayout()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Reset();
	BossReturnPortal = nullptr;

	if (FloorISM)   { FloorISM->ClearInstances(); }
	if (WallISM)    { WallISM->ClearInstances(); }
	if (CeilingISM) { CeilingISM->ClearInstances(); }
	if (TorchISM)   { TorchISM->ClearInstances(); }

	Rooms.Reset();
	BossRoomIndex = INDEX_NONE;

	Cells.Init(ECell::Empty, GridWidth * GridHeight);
}

void ADungeonGenerator::PlaceRooms()
{
	const int32 TotalAttempts = TargetRoomCount * MaxPlacementAttempts;
	const int32 MinSize = FMath::Max(2, FMath::Min(MinRoomCells, MaxRoomCells));
	const int32 MaxSize = FMath::Max(MinSize, MaxRoomCells);

	for (int32 Attempt = 0; Attempt < TotalAttempts && Rooms.Num() < TargetRoomCount; ++Attempt)
	{
		FDungeonRoom Room;
		Room.W = Rng.RandRange(MinSize, MaxSize);
		Room.H = Rng.RandRange(MinSize, MaxSize);
		// Leave a 1-cell border so outer walls always have room to build.
		Room.X = Rng.RandRange(1, GridWidth - Room.W - 1);
		Room.Y = Rng.RandRange(1, GridHeight - Room.H - 1);

		if (Room.X < 1 || Room.Y < 1)
		{
			continue;
		}

		// Reject if it touches an existing room (expand by 1 cell so rooms never share a wall).
		bool bOverlaps = false;
		for (const FDungeonRoom& Other : Rooms)
		{
			const bool bSeparated =
				Room.X > Other.X + Other.W ||
				Other.X > Room.X + Room.W ||
				Room.Y > Other.Y + Other.H ||
				Other.Y > Room.Y + Room.H;
			if (!bSeparated)
			{
				bOverlaps = true;
				break;
			}
		}
		if (bOverlaps)
		{
			continue;
		}

		for (int32 y = Room.Y; y < Room.Y + Room.H; ++y)
		{
			for (int32 x = Room.X; x < Room.X + Room.W; ++x)
			{
				Cells[Idx(x, y)] = ECell::Room;
			}
		}
		Rooms.Add(Room);
	}
}

void ADungeonGenerator::PlaceBossRoom()
{
	if (Rooms.Num() == 0)
	{
		return; // need a start room to measure distance from
	}

	const int32 Size = FMath::Clamp(BossRoomCells, 6, FMath::Min(GridWidth, GridHeight) - 2);
	const FDungeonRoom& Start = Rooms[0];
	const FVector2D StartCenter(Start.CenterX(), Start.CenterY());

	// Search placements and keep the valid one furthest from the player's start room.
	FDungeonRoom Best;
	float BestDistSq = -1.f;
	const int32 Attempts = FMath::Max(64, MaxPlacementAttempts);

	for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
	{
		FDungeonRoom Room;
		Room.W = Size;
		Room.H = Size;
		Room.X = Rng.RandRange(1, GridWidth - Room.W - 1);
		Room.Y = Rng.RandRange(1, GridHeight - Room.H - 1);
		if (Room.X < 1 || Room.Y < 1)
		{
			continue;
		}

		bool bOverlaps = false;
		for (const FDungeonRoom& Other : Rooms)
		{
			const bool bSeparated =
				Room.X > Other.X + Other.W ||
				Other.X > Room.X + Room.W ||
				Room.Y > Other.Y + Other.H ||
				Other.Y > Room.Y + Room.H;
			if (!bSeparated)
			{
				bOverlaps = true;
				break;
			}
		}
		if (bOverlaps)
		{
			continue;
		}

		const float DistSq = FVector2D::DistSquared(FVector2D(Room.CenterX(), Room.CenterY()), StartCenter);
		if (DistSq > BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Room;
		}
	}

	if (BestDistSq < 0.f)
	{
		return; // couldn't fit a boss room this layout
	}

	for (int32 y = Best.Y; y < Best.Y + Best.H; ++y)
	{
		for (int32 x = Best.X; x < Best.X + Best.W; ++x)
		{
			Cells[Idx(x, y)] = ECell::Room;
		}
	}
	// Added last so the corridor chain ends here, making it the final room before the boss.
	Rooms.Add(Best);
	BossRoomIndex = Rooms.Num() - 1;
}

void ADungeonGenerator::CarveCorridors()
{
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		const FDungeonRoom& A = Rooms[i - 1];
		const FDungeonRoom& B = Rooms[i];
		CarveLine(A.CenterX(), A.CenterY(), B.CenterX(), B.CenterY(), Rng.RandRange(0, 1) == 0);
	}
}

void ADungeonGenerator::CarveLine(int32 X0, int32 Y0, int32 X1, int32 Y1, bool bHorizontalFirst)
{
	auto Carve = [this](int32 X, int32 Y)
	{
		if (InBounds(X, Y) && Cells[Idx(X, Y)] == ECell::Empty)
		{
			Cells[Idx(X, Y)] = ECell::Corridor;
		}
	};

	const int32 CornerX = bHorizontalFirst ? X1 : X0;
	const int32 CornerY = bHorizontalFirst ? Y0 : Y1;

	// First leg.
	for (int32 x = FMath::Min(X0, CornerX); x <= FMath::Max(X0, CornerX); ++x)
	{
		Carve(x, Y0);
	}
	for (int32 y = FMath::Min(Y0, CornerY); y <= FMath::Max(Y0, CornerY); ++y)
	{
		Carve(X0, y);
	}
	// Second leg, into the destination.
	for (int32 x = FMath::Min(CornerX, X1); x <= FMath::Max(CornerX, X1); ++x)
	{
		Carve(x, Y1);
	}
	for (int32 y = FMath::Min(CornerY, Y1); y <= FMath::Max(CornerY, Y1); ++y)
	{
		Carve(X1, y);
	}
}

void ADungeonGenerator::AddTile(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& SizeCm)
{
	if (!ISM)
	{
		return;
	}
	const FTransform Xf(FRotator::ZeroRotator, Center, SizeCm / GenUnitCm);
	ISM->AddInstance(Xf, /*bWorldSpace*/ false);
}

void ADungeonGenerator::BuildGeometry()
{
	const float HalfCell = CellSize * 0.5f;
	// Wall segments span exactly one cell. (Spanning *past* the cell edge would make collinear
	// segments along a straight run overlap each other, which z-fights. Interior corners are still
	// fully covered by the perpendicular segment's thickness.)
	const float WallSpan = CellSize;

	// If a custom floor mesh exists, drive FloorISM with it. Auto-fit any authored size: uniform
	// scale so its footprint fills the cell (no stretch), centered, with its top at Z = 0.
	UStaticMesh* FloorMesh = Cast<UStaticMesh>(FSoftObjectPath(FloorMeshPath).TryLoad());
	const bool bCustomFloor = (FloorMesh != nullptr) && (FloorISM != nullptr);
	float FloorScale = 1.f;
	FVector FloorOffset = FVector::ZeroVector;
	// Ceiling reuses the floor mesh, NOT flipped — so the player sees its underside (which can be
	// textured differently for variety). The mesh's bottom face is placed at the ceiling plane.
	FVector FloorCtr = FVector::ZeroVector;
	float FloorMaxZ = 0.f;
	float FloorMinZ = 0.f;
	if (bCustomFloor)
	{
		FloorISM->SetStaticMesh(FloorMesh);
		CeilingISM->SetStaticMesh(FloorMesh);
		const FBox Box = FloorMesh->GetBoundingBox();
		const FVector Size = Box.GetSize();
		FloorCtr = Box.GetCenter();
		FloorMaxZ = Box.Max.Z;
		FloorMinZ = Box.Min.Z;
		const float Denom = FMath::Max(Size.X, Size.Y);
		FloorScale = (Denom > KINDA_SMALL_NUMBER) ? (CellSize / Denom) : 1.f;
		// Center the mesh on the cell (XY) and land its top face at Z = 0.
		FloorOffset = FVector(-FloorCtr.X * FloorScale, -FloorCtr.Y * FloorScale, -FloorMaxZ * FloorScale);
	}

	// If a custom wall mesh exists, drive WallISM with it. Auto-fit: scale its length to the cell, its
	// thickness to WallThickness, and its height to WallHeight; rotate so its length runs along the
	// wall. (Falls back to the graybox cube wall otherwise.)
	UStaticMesh* WallMesh = Cast<UStaticMesh>(FSoftObjectPath(WallMeshPath).TryLoad());
	const bool bCustomWall = (WallMesh != nullptr) && (WallISM != nullptr);
	bool bWallLenAlongX = true;
	FVector WallScale = FVector::OneVector;
	FBox WallBox(ForceInit);
	// Extend the wall past the floor and ceiling by this much so its top/bottom faces are never
	// coplanar with the slabs — that seam is what lets Lumen leak a bright band along the wall tops.
	const float WallEmbed = 30.f;
	if (bCustomWall)
	{
		WallISM->SetStaticMesh(WallMesh);
		WallBox = WallMesh->GetBoundingBox();
		const FVector Sz = WallBox.GetSize();
		bWallLenAlongX = (Sz.X >= Sz.Y); // longer horizontal axis is the wall's length
		const float ZScale = (WallHeight + 2.f * WallEmbed) / Sz.Z; // embed into both slabs
		if (bWallLenAlongX)
		{
			WallScale = FVector(CellSize / Sz.X, WallThickness / Sz.Y, ZScale);
		}
		else
		{
			WallScale = FVector(WallThickness / Sz.X, CellSize / Sz.Y, ZScale);
		}
	}

	// Places one custom wall instance centered on an edge midpoint, bottom embedded at -WallEmbed (so
	// it overlaps floor and ceiling), length running along X or Y. bFlip adds 180° yaw for variety.
	auto AddWall = [&](const FVector& EdgeMid, bool bRunAlongX, bool bFlip)
	{
		float Yaw = bRunAlongX ? (bWallLenAlongX ? 0.f : 90.f) : (bWallLenAlongX ? 90.f : 0.f);
		if (bFlip) { Yaw += 180.f; }
		const FRotator Rot(0.f, Yaw, 0.f);
		const FVector CRot = Rot.RotateVector(WallBox.GetCenter() * WallScale);
		const FVector Loc(EdgeMid.X - CRot.X, EdgeMid.Y - CRot.Y, -WallBox.Min.Z * WallScale.Z - WallEmbed);
		WallISM->AddInstance(FTransform(Rot, Loc, WallScale), /*bWorldSpace*/ false);
	};

	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (!IsFloor(x, y))
			{
				continue;
			}

			const FVector C = CellToLocal(x, y);

			// Floor: top sits at Z = 0. (Collision comes from the mesh itself — cube or imported.)
			if (bCustomFloor)
			{
				FloorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FloorOffset, FVector(FloorScale)), /*bWorldSpace*/ false);
			}
			else
			{
				AddTile(FloorISM, C + FVector(0.f, 0.f, -SlabThickness * 0.5f),
					FVector(CellSize, CellSize, SlabThickness));
			}

			// Ceiling: reuse the floor mesh UN-flipped (its bottom face shows from below) with that
			// bottom face sitting at the ceiling plane Z = WallHeight; else a cube.
			if (bCustomFloor)
			{
				const FVector CeilLoc = C + FVector(-FloorCtr.X * FloorScale, -FloorCtr.Y * FloorScale,
					WallHeight - FloorMinZ * FloorScale);
				CeilingISM->AddInstance(FTransform(FRotator::ZeroRotator, CeilLoc, FVector(FloorScale)), /*bWorldSpace*/ false);
			}
			else
			{
				AddTile(CeilingISM, C + FVector(0.f, 0.f, WallHeight + SlabThickness * 0.5f),
					FVector(CellSize, CellSize, SlabThickness));
			}

			// Cube-wall geometry (used when there's no custom wall mesh): embed into both slabs so no
			// wall face is coplanar with floor-top/ceiling-bottom (avoids z-fighting).
			const float WallTall = WallHeight + 2.f * SlabThickness;
			const float WallZ = WallHeight * 0.5f;

			// Raise a wall on each edge that borders a non-floor cell (doorways are where two
			// floor cells meet, so no wall is built there). Custom walls alternate facing (flip every
			// other cell along the run) so the painted side varies.
			if (!IsFloor(x + 1, y))
			{
				if (bCustomWall) { AddWall(C + FVector(HalfCell, 0.f, 0.f), /*bRunAlongX*/ false, (y & 1) != 0); }
				else { AddTile(WallISM, C + FVector(HalfCell, 0.f, WallZ), FVector(WallThickness, WallSpan, WallTall)); }
			}
			if (!IsFloor(x - 1, y))
			{
				if (bCustomWall) { AddWall(C + FVector(-HalfCell, 0.f, 0.f), /*bRunAlongX*/ false, (y & 1) == 0); }
				else { AddTile(WallISM, C + FVector(-HalfCell, 0.f, WallZ), FVector(WallThickness, WallSpan, WallTall)); }
			}
			if (!IsFloor(x, y + 1))
			{
				if (bCustomWall) { AddWall(C + FVector(0.f, HalfCell, 0.f), /*bRunAlongX*/ true, (x & 1) != 0); }
				else { AddTile(WallISM, C + FVector(0.f, HalfCell, WallZ), FVector(WallSpan, WallThickness, WallTall)); }
			}
			if (!IsFloor(x, y - 1))
			{
				if (bCustomWall) { AddWall(C + FVector(0.f, -HalfCell, 0.f), /*bRunAlongX*/ true, (x & 1) == 0); }
				else { AddTile(WallISM, C + FVector(0.f, -HalfCell, WallZ), FVector(WallSpan, WallThickness, WallTall)); }
			}
		}
	}
}

void ADungeonGenerator::ScatterProps()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Only furniture that has a finished imported mesh is spawned (the graybox-only types are left
	// out of generation). Add a type here once its SM_ asset exists.
	static const EPropType ModeledProps[] = { EPropType::Stool, EPropType::Table, EPropType::Crate };
	const int32 NumModeled = UE_ARRAY_COUNT(ModeledProps);

	// Test lineup: place ONE of every modeled furniture type in the player's start room (room 0), in
	// a row in front of where the player spawns (room center, facing +X), so you can eyeball them.
	if (Rooms.Num() > 0)
	{
		const FVector Base = GetRoomCenterWorld(0);
		ADungeonProp* TableProp = nullptr;
		for (int32 i = 0; i < NumModeled; ++i)
		{
			const FVector Offset(260.f, (i - (NumModeled - 1) * 0.5f) * 200.f, 0.f);
			if (ADungeonProp* Prop = World->SpawnActor<ADungeonProp>(
				ADungeonProp::StaticClass(), FTransform(FRotator::ZeroRotator, Base + Offset), Params))
			{
				Prop->Configure(ModeledProps[i]);
				SpawnedActors.Add(Prop);
				if (ModeledProps[i] == EPropType::Table)
				{
					TableProp = Prop;
				}
			}
		}

		// Test display: one of each potion on the table top so the recolored potion meshes are
		// visible (and grabbable) the instant the game starts.
		if (TableProp)
		{
			FVector Origin, Extent;
			TableProp->GetActorBounds(/*bOnlyColliding=*/false, Origin, Extent);
			const float TopZ = Origin.Z + Extent.Z;
			static const TCHAR* Potions[] = { TEXT("HealthPotion"), TEXT("ManaPotion"), TEXT("StaminaPotion") };
			const int32 NumPotions = UE_ARRAY_COUNT(Potions);
			for (int32 i = 0; i < NumPotions; ++i)
			{
				const FVector Loc(Origin.X, Origin.Y + (i - (NumPotions - 1) * 0.5f) * 35.f, TopZ + 1.f);
				if (AItemPickup* Pickup = World->SpawnActor<AItemPickup>(
					AItemPickup::StaticClass(), FTransform(FRotator::ZeroRotator, Loc), Params))
				{
					Pickup->Configure(FName(Potions[i]), 1);
					SpawnedActors.Add(Pickup);
				}
			}
		}
	}

	// Random scatter through the other rooms (skip room 0 so the test lineup stays clean).
	for (int32 RoomIndex = 1; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		const FDungeonRoom& Room = Rooms[RoomIndex];

		// Only interior cells (skip the perimeter ring so props avoid walls and doorways).
		for (int32 y = Room.Y + 1; y < Room.Y + Room.H - 1; ++y)
		{
			for (int32 x = Room.X + 1; x < Room.X + Room.W - 1; ++x)
			{
				if (Rng.FRand() > PropFillChance)
				{
					continue;
				}

				const FVector WorldLoc = GetActorTransform().TransformPosition(CellToLocal(x, y));
				const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);

				ADungeonProp* Prop = World->SpawnActor<ADungeonProp>(
					ADungeonProp::StaticClass(), FTransform(Rot, WorldLoc), Params);
				if (Prop)
				{
					Prop->Configure(ModeledProps[Rng.RandRange(0, NumModeled - 1)]);
					SpawnedActors.Add(Prop);
				}
			}
		}
	}
}

void ADungeonGenerator::ScatterMonsters()
{
	UWorld* World = GetWorld();
	if (!World || !MonsterClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	// Capsules need room; nudge them clear of each other rather than refusing to spawn.
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const int32 MinSize = FMath::Max(1, FMath::Min(MinGroupSize, MaxGroupSize));
	const int32 MaxSize = FMath::Max(MinSize, MaxGroupSize);

	// Skip room 0 (player spawn) and the boss room (the boss fights solo).
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		if (i == BossRoomIndex || Rng.FRand() > MonsterGroupChance)
		{
			continue;
		}

		const FDungeonRoom& Room = Rooms[i];
		const FVector Center = GetRoomCenterWorld(i);
		const float SpreadX = FMath::Max(0.f, (Room.W - 1) * 0.4f) * CellSize;
		const float SpreadY = FMath::Max(0.f, (Room.H - 1) * 0.4f) * CellSize;

		const int32 GroupSize = Rng.RandRange(MinSize, MaxSize);
		for (int32 k = 0; k < GroupSize; ++k)
		{
			const FVector Offset(Rng.FRandRange(-SpreadX, SpreadX), Rng.FRandRange(-SpreadY, SpreadY), 100.f);
			const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);

			if (AMonsterCharacter* Monster = World->SpawnActor<AMonsterCharacter>(
				MonsterClass, FTransform(Rot, Center + Offset), Params))
			{
				Monster->ApplyType(MonsterDatabase::RollRandomType(Rng));
				SpawnedActors.Add(Monster);
			}
		}
	}
}

void ADungeonGenerator::ScatterChests()
{
	UWorld* World = GetWorld();
	if (!World || !ChestClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Always drop one chest in the start room (room 0), off to the side of the furniture lineup, so
	// it's testable the instant the game starts.
	if (Rooms.Num() > 0)
	{
		const FVector Loc = GetRoomCenterWorld(0) + FVector(CellSize * 1.3f, CellSize * 0.8f, 0.f);
		if (ALootChest* Chest = World->SpawnActor<ALootChest>(ChestClass, FTransform(FRotator::ZeroRotator, Loc), Params))
		{
			SpawnedActors.Add(Chest);
		}
	}

	// Skip the player's start room; the boss room always gets one as a reward.
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		const bool bBossRoom = (i == BossRoomIndex);
		if (!bBossRoom && Rng.FRand() > ChestChancePerRoom)
		{
			continue;
		}

		const FDungeonRoom& Room = Rooms[i];
		const float SpreadX = FMath::Max(0.f, (Room.W - 2) * 0.4f) * CellSize;
		const float SpreadY = FMath::Max(0.f, (Room.H - 2) * 0.4f) * CellSize;
		const FVector Offset(Rng.FRandRange(-SpreadX, SpreadX), Rng.FRandRange(-SpreadY, SpreadY), 0.f);
		const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);

		if (ALootChest* Chest = World->SpawnActor<ALootChest>(ChestClass, FTransform(Rot, GetRoomCenterWorld(i) + Offset), Params))
		{
			SpawnedActors.Add(Chest);
		}
	}
}

void ADungeonGenerator::SpawnBoss()
{
	UWorld* World = GetWorld();
	if (!World || !BossClass || BossRoomIndex == INDEX_NONE)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Lift by the boss capsule half-height so it stands on the floor.
	const FVector Loc = GetRoomCenterWorld(BossRoomIndex) + FVector(0.f, 0.f, 160.f);
	if (ABossMonster* Boss = World->SpawnActor<ABossMonster>(BossClass, FTransform(Loc), Params))
	{
		SpawnedActors.Add(Boss);

		// Reward portal in the boss room: spawned dormant, activated when the boss dies.
		TSubclassOf<APortal> PClass = PortalClass;
		if (!PClass) { PClass = APortal::StaticClass(); }
		const FVector PortalLoc = GetRoomCenterWorld(BossRoomIndex) + FVector(0.f, CellSize * 1.5f, 0.f);
		if (APortal* Portal = World->SpawnActor<APortal>(PClass, FTransform(FRotator::ZeroRotator, PortalLoc), Params))
		{
			Portal->SetTargetMapName(TownMapName);
			Portal->SetActive(false);
			BossReturnPortal = Portal;
			SpawnedActors.Add(Portal);
		}

		if (UHealthComponent* BossHealth = Boss->FindComponentByClass<UHealthComponent>())
		{
			BossHealth->OnDepleted.AddUObject(this, &ADungeonGenerator::HandleBossDefeated);
		}
	}
}

void ADungeonGenerator::SpawnReturnPortals()
{
	UWorld* World = GetWorld();
	if (!World || Rooms.Num() == 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<APortal> PClass = PortalClass;
	if (!PClass) { PClass = APortal::StaticClass(); }

	// Place the return portal flush against a SOLID wall on the back (-X) side of the start room — the
	// side opposite the furniture/chest lineup (+X). Scan that edge for a wall cell (a floor cell whose
	// -X neighbour is non-floor) nearest the room's vertical center, skipping doorways.
	const FDungeonRoom& Start = Rooms[0];
	const int32 WallX = Start.X;            // leftmost interior column of the room
	const int32 CenterY = Start.CenterY();
	int32 BestY = INDEX_NONE;
	int32 BestDist = MAX_int32;
	for (int32 y = Start.Y; y < Start.Y + Start.H; ++y)
	{
		if (IsFloor(WallX, y) && !IsFloor(WallX - 1, y)) // solid wall to -X (not a doorway)
		{
			const int32 Dist = FMath::Abs(y - CenterY);
			if (Dist < BestDist) { BestDist = Dist; BestY = y; }
		}
	}

	FVector PortalLocal;
	FRotator PortalRot = FRotator::ZeroRotator; // thin slab axis is X — sits flush on an X-facing wall
	if (BestY != INDEX_NONE)
	{
		PortalLocal = CellToLocal(WallX, BestY);
		PortalLocal.X -= CellSize * 0.5f - 40.f; // hug the wall, leaving a little clearance
	}
	else
	{
		PortalLocal = CellToLocal(Start.CenterX(), Start.CenterY()) + FVector(-CellSize * 1.2f, 0.f, 0.f);
	}

	const FVector Loc = GetActorTransform().TransformPosition(PortalLocal);
	if (APortal* Portal = World->SpawnActor<APortal>(PClass, FTransform(PortalRot, Loc), Params))
	{
		Portal->SetTargetMapName(TownMapName);
		Portal->SetActive(true);
		SpawnedActors.Add(Portal);
	}
}

void ADungeonGenerator::HandleBossDefeated(UHealthComponent* /*DeadComponent*/)
{
	if (BossReturnPortal)
	{
		BossReturnPortal->SetActive(true);
	}
}

void ADungeonGenerator::SpawnWallTorches()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 Spacing = FMath::Max(1, TorchSpacingCells);
	const int32 Stride = Spacing * 2; // half as many torches as before

	// Walk every floor cell; on edges that border a non-floor cell (a wall) place a torch. Torches
	// sit every Stride cells, and the two facing walls are offset by Spacing so they alternate /
	// stagger down the corridor instead of lining up.
	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (!IsFloor(x, y))
			{
				continue;
			}

			const FVector C = CellToLocal(x, y);

			if (!IsFloor(x + 1, y) && (y % Stride) == 0)       { PlaceWallTorch(C, FVector(1.f, 0.f, 0.f)); }
			if (!IsFloor(x - 1, y) && (y % Stride) == Spacing) { PlaceWallTorch(C, FVector(-1.f, 0.f, 0.f)); }
			if (!IsFloor(x, y + 1) && (x % Stride) == 0)       { PlaceWallTorch(C, FVector(0.f, 1.f, 0.f)); }
			if (!IsFloor(x, y - 1) && (x % Stride) == Spacing) { PlaceWallTorch(C, FVector(0.f, -1.f, 0.f)); }
		}
	}
}

void ADungeonGenerator::PlaceWallTorch(const FVector& CellLocal, const FVector& OutwardDir)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float HalfCell = CellSize * 0.5f;
	const float TorchZ = WallHeight * 0.4f;

	// Mount point flush on the inner wall face, at mid wall height.
	const FVector MountLocal = CellLocal + OutwardDir * (HalfCell - 4.f) + FVector(0.f, 0.f, TorchZ);
	// The torch's flame is just in front of (into the room) and above the mount; light sits there.
	const FVector FlameLocal = MountLocal - OutwardDir * 14.f + FVector(0.f, 0.f, 22.f);

	if (TorchMesh && TorchISM)
	{
		// The torch's front is along its local -Y, so point local +Y AT the wall (front faces the room).
		const FVector IntoRoom = -OutwardDir;
		const FRotator TorchRot(0.f, FMath::RadiansToDegrees(FMath::Atan2(OutwardDir.Y, OutwardDir.X)) - 90.f, 0.f);

		// Scale the mesh to a sensible height regardless of its imported native scale.
		const FBoxSphereBounds B = TorchMesh->GetBounds();
		const float NativeHeight = FMath::Max(1.f, B.BoxExtent.Z * 2.f);
		const float Scale = FMath::Clamp(TorchMeshHeight / NativeHeight, 0.01f, 50.f);

		// Mount on the wall SURFACE and push the (scaled) mesh into the room so it sits flush on the
		// wall instead of buried in it (abs offset works regardless of where the pivot sits).
		const float OutOffset = (FMath::Abs(B.Origin.Y) + B.BoxExtent.Y) * Scale;
		const FVector WallFace = CellLocal + OutwardDir * HalfCell + FVector(0.f, 0.f, TorchZ);
		const FVector Pos = WallFace + IntoRoom * OutOffset;

		TorchISM->AddInstance(FTransform(TorchRot, Pos, FVector(Scale)), /*bWorldSpace*/ false);
	}
	else
	{
		// Graybox fallback: bracket cube on the wall + a flame nub in front of it.
		AddTile(TorchISM, MountLocal, FVector(14.f, 14.f, 10.f));
		AddTile(TorchISM, FlameLocal, FVector(9.f, 9.f, 14.f));
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector LightWorld = GetActorTransform().TransformPosition(FlameLocal);
	if (APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(LightWorld), Params))
	{
		if (UPointLightComponent* LC = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			LC->SetMobility(EComponentMobility::Movable);
			LC->SetIntensity(TorchLightIntensity);
			LC->SetAttenuationRadius(TorchLightRadius);
			LC->SetLightColor(FLinearColor(1.0f, 0.55f, 0.2f)); // deep amber medieval flame
			LC->SetSourceRadius(14.f);
			LC->SetSoftSourceRadius(40.f);
			// Many torches: keep them as cheap, soft fill (no per-light shadows). Lumen + the sky
			// light give depth; this avoids dozens of shadow-casting movable lights.
			LC->SetCastShadows(false);
		}
		SpawnedActors.Add(Light);
	}
}
