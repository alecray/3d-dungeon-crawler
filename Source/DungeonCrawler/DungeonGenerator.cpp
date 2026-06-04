#include "DungeonGenerator.h"
#include "DungeonProp.h"
#include "MonsterCharacter.h"
#include "MonsterTypes.h"
#include "BossMonster.h"
#include "LootChest.h"
#include "ItemPickup.h"
#include "Portal.h"
#include "DungeonTrap.h"
#include "BossArena.h"
#include "HealthComponent.h"

#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
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
	TrapClass = ADungeonTrap::StaticClass();
	ArenaClass = ABossArena::StaticClass();
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
	AssignRoomTypes();
	CarveCorridors();
	FindBossEntrance();
	BuildGeometry();
	ScatterProps();
	ScatterMonsters();
	ScatterChests();
	ScatterTraps();
	DecorateRooms();
	SetupBossEncounter();
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

	if (FloorISM)   { FloorISM->ClearInstances(); }
	if (WallISM)    { WallISM->ClearInstances(); }
	if (CeilingISM) { CeilingISM->ClearInstances(); }
	if (TorchISM)   { TorchISM->ClearInstances(); }

	Rooms.Reset();
	BossRoomIndex = INDEX_NONE;
	BossDoorX = BossDoorY = BossDoorDir = INDEX_NONE;

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

void ADungeonGenerator::AssignRoomTypes()
{
	// Weighted roll per room (start room 0 and the boss room stay Normal/untagged).
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		if (i == BossRoomIndex)
		{
			continue;
		}
		const float R = Rng.FRand();
		if      (R < 0.14f) { Rooms[i].Type = ERoomType::Treasure; }
		else if (R < 0.28f) { Rooms[i].Type = ERoomType::Ambush; }
		else if (R < 0.40f) { Rooms[i].Type = ERoomType::Rest; }
		else if (R < 0.48f) { Rooms[i].Type = ERoomType::Elite; }
		else                { Rooms[i].Type = ERoomType::Normal; } // ~52%
	}
}

void ADungeonGenerator::FindBossEntrance()
{
	BossDoorX = BossDoorY = BossDoorDir = INDEX_NONE;
	if (!Rooms.IsValidIndex(BossRoomIndex))
	{
		return;
	}

	// Pick the first boss-room perimeter cell that borders a corridor — that's the single entrance; every
	// other perimeter edge gets sealed solid (BuildGeometry) so the arena is fully enclosed. Any such cell
	// is reachable since all corridors form one connected network.
	static const int32 DX[4] = { 1, -1, 0, 0 };
	static const int32 DY[4] = { 0, 0, 1, -1 };
	const FDungeonRoom& BR = Rooms[BossRoomIndex];
	for (int32 y = BR.Y; y < BR.Y + BR.H; ++y)
	{
		for (int32 x = BR.X; x < BR.X + BR.W; ++x)
		{
			if (CellAt(x, y) != ECell::Room)
			{
				continue;
			}
			for (int32 d = 0; d < 4; ++d)
			{
				if (CellAt(x + DX[d], y + DY[d]) == ECell::Corridor)
				{
					BossDoorX = x;
					BossDoorY = y;
					BossDoorDir = d;
					return;
				}
			}
		}
	}
}

void ADungeonGenerator::DecorateRooms()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// A colored marker light at each special room's center so its type reads at a glance.
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		if (i == BossRoomIndex)
		{
			continue;
		}
		FLinearColor Color;
		float Intensity;
		switch (Rooms[i].Type)
		{
		case ERoomType::Treasure: Color = FLinearColor(1.0f, 0.78f, 0.25f); Intensity = 4000.f; break; // gold
		case ERoomType::Elite:    Color = FLinearColor(0.7f, 0.30f, 1.00f); Intensity = 4000.f; break; // purple
		case ERoomType::Rest:     Color = FLinearColor(0.4f, 1.00f, 0.55f); Intensity = 2500.f; break; // green
		case ERoomType::Ambush:   Color = FLinearColor(1.0f, 0.22f, 0.15f); Intensity = 3000.f; break; // red
		default: continue; // Normal rooms get no marker
		}

		const FVector Loc = GetRoomCenterWorld(i) + FVector(0.f, 0.f, 250.f);
		if (APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(Loc), Params))
		{
			if (UPointLightComponent* C = Cast<UPointLightComponent>(Light->GetLightComponent()))
			{
				C->SetMobility(EComponentMobility::Movable);
				C->SetLightColor(Color);
				C->SetIntensity(Intensity);
				C->SetAttenuationRadius(800.f);
				C->SetCastShadows(false);
			}
			SpawnedActors.Add(Light);
		}
	}
}

void ADungeonGenerator::CarveCorridors()
{
	// Connect each room to its NEAREST already-placed room rather than the previous one in sequence —
	// this yields shorter, less sprawling corridors while still guaranteeing a fully connected layout.
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		const FDungeonRoom& B = Rooms[i];
		int32 Best = 0;
		int32 BestDistSq = MAX_int32;
		for (int32 j = 0; j < i; ++j)
		{
			const int32 dx = Rooms[j].CenterX() - B.CenterX();
			const int32 dy = Rooms[j].CenterY() - B.CenterY();
			const int32 DistSq = dx * dx + dy * dy;
			if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = j; }
		}
		const FDungeonRoom& A = Rooms[Best];
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

			// The boss room is built double-height (taller walls + raised ceiling) for a grand arena.
			const bool bBoss = IsBossRoomCell(x, y);
			const float CellCeil = bBoss ? WallHeight * 2.f : WallHeight;

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
					CellCeil - FloorMinZ * FloorScale);
				CeilingISM->AddInstance(FTransform(FRotator::ZeroRotator, CeilLoc, FVector(FloorScale)), /*bWorldSpace*/ false);
			}
			else
			{
				AddTile(CeilingISM, C + FVector(0.f, 0.f, CellCeil + SlabThickness * 0.5f),
					FVector(CellSize, CellSize, SlabThickness));
			}

			// Cube-wall geometry (embed into both slabs so no wall face is coplanar with the floor/ceiling).
			static const int32 DX4[4] = { 1, -1, 0, 0 };
			static const int32 DY4[4] = { 0, 0, 1, -1 };
			const float WallZ = WallHeight * 0.5f;
			const float WallTall = WallHeight + 2.f * SlabThickness;

			// Adds one wall course (a cube band) on an edge spanning Z [Bottom, Top].
			auto Course = [&](const FVector& EdgeOff, bool bRunAlongX, float Bottom, float Top)
			{
				const float H = Top - Bottom;
				const FVector Size = bRunAlongX ? FVector(WallSpan, WallThickness, H) : FVector(WallThickness, WallSpan, H);
				AddTile(WallISM, C + EdgeOff + FVector(0.f, 0.f, (Bottom + Top) * 0.5f), Size);
			};

			auto EdgeWall = [&](int32 d, const FVector& EdgeOff, bool bRunAlongX, bool bFlip)
			{
				const int32 nx = x + DX4[d];
				const int32 ny = y + DY4[d];

				if (!bBoss)
				{
					// Normal cell: a single-course wall only where it borders a non-floor cell.
					if (!IsFloor(nx, ny))
					{
						if (bCustomWall) { AddWall(C + EdgeOff, bRunAlongX, bFlip); }
						else
						{
							const FVector Size = bRunAlongX ? FVector(WallSpan, WallThickness, WallTall) : FVector(WallThickness, WallSpan, WallTall);
							AddTile(WallISM, C + EdgeOff + FVector(0.f, 0.f, WallZ), Size);
						}
					}
					return;
				}

				// Boss room: skip edges between two boss cells (interior). Seal every perimeter edge solid
				// with TWO STACKED courses + a seam ledge, except the single chosen entrance (open doorway
				// with just the upper course as a lintel).
				if (IsBossRoomCell(nx, ny))
				{
					return;
				}
				const bool bEntrance = (x == BossDoorX && y == BossDoorY && d == BossDoorDir);
				// Two plain wall courses stacked floor-to-ceiling (a normal-looking wall, just 2x tall);
				// the single entrance gets only the upper course so its lower half is an open doorway.
				if (!bEntrance)
				{
					Course(EdgeOff, bRunAlongX, -SlabThickness, WallHeight); // lower course
				}
				Course(EdgeOff, bRunAlongX, WallHeight, CellCeil + SlabThickness); // upper course
			};

			EdgeWall(0, FVector(HalfCell, 0.f, 0.f),  /*bRunAlongX*/ false, (y & 1) != 0);
			EdgeWall(1, FVector(-HalfCell, 0.f, 0.f), /*bRunAlongX*/ false, (y & 1) == 0);
			EdgeWall(2, FVector(0.f, HalfCell, 0.f),  /*bRunAlongX*/ true,  (x & 1) != 0);
			EdgeWall(3, FVector(0.f, -HalfCell, 0.f), /*bRunAlongX*/ true,  (x & 1) == 0);
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

	// Free-standing scenery scattered on open floor cells (rocks/mushrooms get a random scale).
	// Coffins are intentionally excluded here — they always go flush against a wall (see below).
	static const EPropType FloorProps[] = {
		EPropType::Stool, EPropType::Table, EPropType::Crate, EPropType::Barrel, EPropType::Pots,
		EPropType::Bucket, EPropType::Rocks, EPropType::Bones, EPropType::Anvil,
		EPropType::Mushrooms
	};
	const int32 NumFloor = UE_ARRAY_COUNT(FloorProps);

	// Wall-hugging scenery used in corridors and against room walls (won't block the path).
	static const EPropType WallProps[] = {
		EPropType::Barrel, EPropType::Pots, EPropType::Bucket, EPropType::Crate,
		EPropType::Bones, EPropType::Rocks, EPropType::Coffin
	};
	const int32 NumWall = UE_ARRAY_COUNT(WallProps);

	static const int32 DX[4] = { 1, -1, 0, 0 };
	static const int32 DY[4] = { 0, 0, 1, -1 };

	const FTransform Xf = GetActorTransform();
	const float HalfCell = CellSize * 0.5f;
	const float BannerChancePerDoor = 0.6f;
	const float WeaponRackChancePerWallCell = 0.03f;
	const float CoffinChancePerWallCell = 0.05f;

	auto SpawnProp = [&](EPropType Type, const FVector& WorldLoc, const FRotator& Rot, float Scale)
	{
		if (ADungeonProp* Prop = World->SpawnActor<ADungeonProp>(
			ADungeonProp::StaticClass(), FTransform(Rot, WorldLoc), Params))
		{
			Prop->Configure(Type);
			if (!FMath::IsNearlyEqual(Scale, 1.f)) { Prop->SetActorScale3D(FVector(Scale)); }
			SpawnedActors.Add(Prop);
		}
	};

	// Finds a random adjacent solid (non-floor) direction for cell (x,y); used to hug props to walls.
	auto FindAdjacentWall = [&](int32 x, int32 y, FVector& OutDir) -> bool
	{
		int32 Order[4] = { 0, 1, 2, 3 };
		for (int32 i = 3; i > 0; --i) { Swap(Order[i], Order[Rng.RandRange(0, i)]); }
		for (int32 k = 0; k < 4; ++k)
		{
			const int32 d = Order[k];
			if (!IsFloor(x + DX[d], y + DY[d]))
			{
				OutDir = FVector(DX[d], DY[d], 0.f);
				return true;
			}
		}
		return false;
	};

	// Places a prop near the wall in direction Outward, facing into the open space. The inset keeps the
	// prop's body clear of the wall thickness so it doesn't clip into the wall.
	auto SpawnWallProp = [&](EPropType Type, int32 x, int32 y, const FVector& Outward, float Scale)
	{
		const FVector Loc = Xf.TransformPosition(CellToLocal(x, y)) + Outward * (HalfCell - 55.f);
		SpawnProp(Type, Loc, (-Outward).Rotation(), Scale);
	};

	// Skip room 0 so the player's spawn room stays clear.
	for (int32 RoomIndex = 1; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		const FDungeonRoom& Room = Rooms[RoomIndex];

		// --- Floor clutter on interior cells (skip the perimeter ring so it avoids walls/doors) ---
		for (int32 y = Room.Y + 1; y < Room.Y + Room.H - 1; ++y)
		{
			for (int32 x = Room.X + 1; x < Room.X + Room.W - 1; ++x)
			{
				if (Rng.FRand() > PropFillChance)
				{
					continue;
				}
				const EPropType Type = FloorProps[Rng.RandRange(0, NumFloor - 1)];
				const float Scale = (Type == EPropType::Rocks || Type == EPropType::Mushrooms)
					? Rng.FRandRange(0.6f, 1.4f) : 1.f;
				const FVector Loc = Xf.TransformPosition(CellToLocal(x, y));
				const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
				SpawnProp(Type, Loc, Rot, Scale);
			}
		}

		// --- Prop clusters: clumps of furniture piled tightly together for a lived-in look ---
		const int32 InteriorCells = FMath::Max(0, Room.W - 2) * FMath::Max(0, Room.H - 2);
		const int32 NumClusters = FMath::RoundToInt(InteriorCells * PropClusterDensity);
		const float ClusterRadius = CellSize * 0.55f; // props pile within roughly one cell of the center
		for (int32 c = 0; c < NumClusters; ++c)
		{
			const int32 ccx = Rng.RandRange(Room.X + 1, Room.X + Room.W - 2);
			const int32 ccy = Rng.RandRange(Room.Y + 1, Room.Y + Room.H - 2);
			const FVector ClusterCenter = Xf.TransformPosition(CellToLocal(ccx, ccy));

			// Each cluster favours one "primary" prop type so it reads as a coherent pile (a stack of
			// barrels, a heap of crates), with the occasional odd item mixed in.
			const EPropType Primary = FloorProps[Rng.RandRange(0, NumFloor - 1)];
			const int32 Count = Rng.RandRange(FMath::Min(ClusterPropsMin, ClusterPropsMax), FMath::Max(ClusterPropsMin, ClusterPropsMax));
			for (int32 p = 0; p < Count; ++p)
			{
				const EPropType Type = (Rng.FRand() < 0.7f) ? Primary : FloorProps[Rng.RandRange(0, NumFloor - 1)];
				const float Scale = (Type == EPropType::Rocks || Type == EPropType::Mushrooms)
					? Rng.FRandRange(0.6f, 1.4f) : 1.f;
				const FVector Offset(Rng.FRandRange(-ClusterRadius, ClusterRadius), Rng.FRandRange(-ClusterRadius, ClusterRadius), 0.f);
				const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
				SpawnProp(Type, ClusterCenter + Offset, Rot, Scale);
			}
		}

		// --- Wall-mounted weapon racks + banners flanking doorways (whole room incl. perimeter) ---
		for (int32 y = Room.Y; y < Room.Y + Room.H; ++y)
		{
			for (int32 x = Room.X; x < Room.X + Room.W; ++x)
			{
				if (!IsFloor(x, y))
				{
					continue;
				}
				for (int32 d = 0; d < 4; ++d)
				{
					const int32 nx = x + DX[d];
					const int32 ny = y + DY[d];
					const FVector Outward(DX[d], DY[d], 0.f);

					if (CellAt(x, y) == ECell::Room && CellAt(nx, ny) == ECell::Corridor)
					{
						// Doorway: banners on the room-side wall flanking the opening.
						if (Rng.FRand() <= BannerChancePerDoor)
						{
							const int32 PX = -DY[d];
							const int32 PY = DX[d];
							for (int32 s = -1; s <= 1; s += 2)
							{
								const int32 jx = x + s * PX;
								const int32 jy = y + s * PY;
								if (IsFloor(jx, jy) && !IsFloor(jx + DX[d], jy + DY[d]))
								{
									const FVector Loc = Xf.TransformPosition(CellToLocal(jx, jy)) + Outward * (HalfCell - 12.f);
									SpawnProp(EPropType::Banner, Loc, (-Outward).Rotation(), 1.f);
								}
							}
						}
						break; // this cell handled as a doorway
					}
					if (!IsFloor(nx, ny))
					{
						if (Rng.FRand() <= WeaponRackChancePerWallCell)
						{
							// Solid wall: mount a weapon rack flush against it, facing into the room.
							const FVector Loc = Xf.TransformPosition(CellToLocal(x, y)) + Outward * (HalfCell - 18.f);
							SpawnProp(EPropType::WeaponRack, Loc, (-Outward).Rotation(), 1.f);
							break; // one wall prop per cell
						}
						if (Rng.FRand() <= CoffinChancePerWallCell)
						{
							// Solid wall: stand a coffin flush against it, facing into the room.
							SpawnWallProp(EPropType::Coffin, x, y, Outward, 1.f);
							break; // one wall prop per cell
						}
					}
				}
			}
		}
	}

	// --- Corridor clutter: wall-hugging props so they never block the passage ---
	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (CellAt(x, y) != ECell::Corridor || Rng.FRand() > CorridorPropChance)
			{
				continue;
			}
			FVector Outward;
			if (!FindAdjacentWall(x, y, Outward))
			{
				continue; // open junction cell — leave it clear
			}
			const EPropType Type = WallProps[Rng.RandRange(0, NumWall - 1)];
			const float Scale = (Type == EPropType::Rocks) ? Rng.FRandRange(0.6f, 1.2f) : 1.f;
			SpawnWallProp(Type, x, y, Outward, Scale);
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
		if (i == BossRoomIndex)
		{
			continue;
		}

		const FDungeonRoom& Room = Rooms[i];
		const ERoomType Type = Room.Type;
		if (Type == ERoomType::Rest)
		{
			continue; // safe breather room
		}

		const FVector Center = GetRoomCenterWorld(i);
		const float SpreadX = FMath::Max(0.f, (Room.W - 1) * 0.4f) * CellSize;
		const float SpreadY = FMath::Max(0.f, (Room.H - 1) * 0.4f) * CellSize;

		auto SpawnMonster = [&](const FVector& Offset) -> AMonsterCharacter*
		{
			const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
			if (AMonsterCharacter* Monster = World->SpawnActor<AMonsterCharacter>(
				MonsterClass, FTransform(Rot, Center + Offset), Params))
			{
				Monster->ApplyType(MonsterDatabase::RollRandomType(Rng));
				SpawnedActors.Add(Monster);
				return Monster;
			}
			return nullptr;
		};

		// Elite room: a single tough mini-boss.
		if (Type == ERoomType::Elite)
		{
			if (AMonsterCharacter* Elite = SpawnMonster(FVector(0.f, 0.f, 100.f)))
			{
				Elite->MakeElite();
			}
			continue;
		}

		// Treasure (guards) and Ambush always populate; normal rooms roll the chance.
		const bool bForced = (Type == ERoomType::Treasure || Type == ERoomType::Ambush);
		if (!bForced && Rng.FRand() > MonsterGroupChance)
		{
			continue;
		}

		int32 GroupSize = Rng.RandRange(MinSize, MaxSize);
		if (Type == ERoomType::Ambush)
		{
			GroupSize = MaxSize + 2; // a bigger swarm
		}

		for (int32 k = 0; k < GroupSize; ++k)
		{
			SpawnMonster(FVector(Rng.FRandRange(-SpreadX, SpreadX), Rng.FRandRange(-SpreadY, SpreadY), 100.f));
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

	// Skip the player's start room (kept clear); the boss room always gets one as a reward.
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		const bool bBossRoom = (i == BossRoomIndex);

		// How many chests this room gets, by type.
		int32 NumChests;
		if (bBossRoom)                              { NumChests = 1; }
		else if (Rooms[i].Type == ERoomType::Treasure) { NumChests = Rng.RandRange(2, 3); }
		else if (Rooms[i].Type == ERoomType::Elite)    { NumChests = 1; }
		else if (Rooms[i].Type == ERoomType::Rest)     { NumChests = 0; }
		else                                        { NumChests = (Rng.FRand() <= ChestChancePerRoom) ? 1 : 0; }

		const FDungeonRoom& Room = Rooms[i];
		const float SpreadX = FMath::Max(0.f, (Room.W - 2) * 0.4f) * CellSize;
		const float SpreadY = FMath::Max(0.f, (Room.H - 2) * 0.4f) * CellSize;

		for (int32 c = 0; c < NumChests; ++c)
		{
			const FVector Offset(Rng.FRandRange(-SpreadX, SpreadX), Rng.FRandRange(-SpreadY, SpreadY), 0.f);
			const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
			if (ALootChest* Chest = World->SpawnActor<ALootChest>(ChestClass, FTransform(Rot, GetRoomCenterWorld(i) + Offset), Params))
			{
				SpawnedActors.Add(Chest);
			}
		}
	}
}

void ADungeonGenerator::ScatterTraps()
{
	UWorld* World = GetWorld();
	if (!World || !TrapClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform Xf = GetActorTransform();
	const float HalfCell = CellSize * 0.5f;

	static const int32 DX[4] = { 1, -1, 0, 0 };
	static const int32 DY[4] = { 0, 0, 1, -1 };

	// Keep the player's start room (room 0) free of hazards.
	auto InStartRoom = [&](int32 x, int32 y) -> bool
	{
		if (!Rooms.IsValidIndex(0)) { return false; }
		const FDungeonRoom& R = Rooms[0];
		return x >= R.X && x < R.X + R.W && y >= R.Y && y < R.Y + R.H;
	};

	auto SpawnTrap = [&](ETrapType Type, const FVector& WorldLoc, const FRotator& Rot)
	{
		if (ADungeonTrap* Trap = World->SpawnActor<ADungeonTrap>(TrapClass, FTransform(Rot, WorldLoc), Params))
		{
			Trap->Configure(Type);
			SpawnedActors.Add(Trap);
		}
	};

	// --- Corridors: floor hazards, and dart shooters mounted to fire across the passage ---
	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (CellAt(x, y) != ECell::Corridor || InStartRoom(x, y))
			{
				continue;
			}

			if (Rng.FRand() <= CorridorTrapChance)
			{
				const ETrapType Type = (Rng.FRand() < 0.5f) ? ETrapType::SpikeFloor : ETrapType::PressurePlate;
				SpawnTrap(Type, Xf.TransformPosition(CellToLocal(x, y)), FRotator::ZeroRotator);
				continue; // don't also mount a shooter on this cell
			}

			if (Rng.FRand() <= DartShooterChance)
			{
				for (int32 d = 0; d < 4; ++d)
				{
					if (!IsFloor(x + DX[d], y + DY[d]))
					{
						const FVector Outward(DX[d], DY[d], 0.f);
						const FVector Loc = Xf.TransformPosition(CellToLocal(x, y)) + Outward * (HalfCell - 10.f);
						SpawnTrap(ETrapType::DartShooter, Loc, (-Outward).Rotation());
						break;
					}
				}
			}
		}
	}

	// --- Rooms: occasional floor hazards on interior cells (skip perimeter ring and the start room) ---
	for (int32 i = 1; i < Rooms.Num(); ++i)
	{
		const FDungeonRoom& Room = Rooms[i];
		if (Room.Type == ERoomType::Rest)
		{
			continue; // safe room — no traps
		}
		for (int32 y = Room.Y + 1; y < Room.Y + Room.H - 1; ++y)
		{
			for (int32 x = Room.X + 1; x < Room.X + Room.W - 1; ++x)
			{
				if (Rng.FRand() <= RoomTrapChance)
				{
					const ETrapType Type = (Rng.FRand() < 0.5f) ? ETrapType::SpikeFloor : ETrapType::PressurePlate;
					SpawnTrap(Type, Xf.TransformPosition(CellToLocal(x, y)), FRotator::ZeroRotator);
				}
			}
		}
	}
}

void ADungeonGenerator::SetupBossEncounter()
{
	UWorld* World = GetWorld();
	if (!World || !BossClass || BossRoomIndex == INDEX_NONE || !Rooms.IsValidIndex(BossRoomIndex))
	{
		return;
	}

	TSubclassOf<ABossArena> AClass = ArenaClass;
	if (!AClass) { AClass = ABossArena::StaticClass(); }

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector RoomCenter = GetRoomCenterWorld(BossRoomIndex);
	ABossArena* Arena = World->SpawnActor<ABossArena>(AClass, FTransform(RoomCenter), Params);
	if (!Arena)
	{
		return;
	}
	SpawnedActors.Add(Arena);

	const FDungeonRoom& Room = Rooms[BossRoomIndex];
	const FVector2D RoomHalf(Room.W * 0.5f * CellSize, Room.H * 0.5f * CellSize);
	TSubclassOf<APortal> PClass = PortalClass;
	if (!PClass) { PClass = APortal::StaticClass(); }
	Arena->Configure(BossClass, RoomCenter, RoomHalf, PClass, TownMapName);

	// Seal just the single boss-room entrance (the rest of the perimeter is built solid in BuildGeometry).
	if (BossDoorDir >= 0)
	{
		static const int32 DX[4] = { 1, -1, 0, 0 };
		static const int32 DY[4] = { 0, 0, 1, -1 };
		const FTransform Xf = GetActorTransform();
		const float HalfCell = CellSize * 0.5f;
		const FVector Outward(DX[BossDoorDir], DY[BossDoorDir], 0.f);
		const FVector DoorLocal = CellToLocal(BossDoorX, BossDoorY) + Outward * HalfCell;
		const bool bAlongX = (DX[BossDoorDir] != 0); // opening faces X: barrier thin in X, spanning Y
		const FVector Size = bAlongX
			? FVector(WallThickness, CellSize, WallHeight)
			: FVector(CellSize, WallThickness, WallHeight);
		Arena->AddDoorSlot(FTransform(FRotator::ZeroRotator, Xf.TransformPosition(DoorLocal)), Size);
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
	const float TorchZ = WallHeight * 0.32f;

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
