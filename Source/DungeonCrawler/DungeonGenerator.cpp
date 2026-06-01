#include "DungeonGenerator.h"
#include "DungeonProp.h"
#include "MonsterCharacter.h"
#include "BossMonster.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// The engine cube is a 100cm cube centered on its origin, so an instance scale of 1.0 == 100cm.
static constexpr float CubeUnitCm = 100.f;

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

	MonsterClass = AMonsterCharacter::StaticClass();
	BossClass = ABossMonster::StaticClass();
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
	SpawnBoss();
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
	const FTransform Xf(FRotator::ZeroRotator, Center, SizeCm / CubeUnitCm);
	ISM->AddInstance(Xf, /*bWorldSpace*/ false);
}

void ADungeonGenerator::BuildGeometry()
{
	const float HalfCell = CellSize * 0.5f;
	// Wall segments span exactly one cell. (Spanning *past* the cell edge would make collinear
	// segments along a straight run overlap each other, which z-fights. Interior corners are still
	// fully covered by the perpendicular segment's thickness.)
	const float WallSpan = CellSize;

	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (!IsFloor(x, y))
			{
				continue;
			}

			const FVector C = CellToLocal(x, y);

			// Floor: top sits at Z = 0.
			AddTile(FloorISM, C + FVector(0.f, 0.f, -SlabThickness * 0.5f),
				FVector(CellSize, CellSize, SlabThickness));

			// Ceiling: bottom sits at Z = WallHeight.
			AddTile(CeilingISM, C + FVector(0.f, 0.f, WallHeight + SlabThickness * 0.5f),
				FVector(CellSize, CellSize, SlabThickness));

			// Embed the wall into both slabs (bottom buried in the floor at -SlabThickness, top buried
			// in the ceiling at WallHeight + SlabThickness) so no wall face is coplanar with the
			// floor-top or ceiling-bottom faces — coplanar overlaps are what cause z-fighting.
			const float WallTall = WallHeight + 2.f * SlabThickness;
			const float WallZ = WallHeight * 0.5f;

			// Raise a wall on each edge that borders a non-floor cell (doorways are where two
			// floor cells meet, so no wall is built there).
			if (!IsFloor(x + 1, y))
			{
				AddTile(WallISM, C + FVector(HalfCell, 0.f, WallZ),
					FVector(WallThickness, WallSpan, WallTall));
			}
			if (!IsFloor(x - 1, y))
			{
				AddTile(WallISM, C + FVector(-HalfCell, 0.f, WallZ),
					FVector(WallThickness, WallSpan, WallTall));
			}
			if (!IsFloor(x, y + 1))
			{
				AddTile(WallISM, C + FVector(0.f, HalfCell, WallZ),
					FVector(WallSpan, WallThickness, WallTall));
			}
			if (!IsFloor(x, y - 1))
			{
				AddTile(WallISM, C + FVector(0.f, -HalfCell, WallZ),
					FVector(WallSpan, WallThickness, WallTall));
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

	// Test lineup: place ONE of every furniture type in the player's start room (room 0), laid out in
	// a grid in front of where the player spawns (room center, facing +X), so you can eyeball every
	// prop the instant the game starts.
	if (Rooms.Num() > 0)
	{
		const FVector Base = GetRoomCenterWorld(0);
		const int32 NumTypes = static_cast<int32>(EPropType::MAX);
		const int32 Cols = 3;
		for (int32 i = 0; i < NumTypes; ++i)
		{
			const int32 Row = i / Cols;
			const int32 Col = i % Cols;
			const FVector Offset(220.f + Row * 190.f, (Col - 1) * 180.f, 0.f);
			if (ADungeonProp* Prop = World->SpawnActor<ADungeonProp>(
				ADungeonProp::StaticClass(), FTransform(FRotator::ZeroRotator, Base + Offset), Params))
			{
				Prop->Configure(static_cast<EPropType>(i));
				SpawnedActors.Add(Prop);
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
					const EPropType Type = static_cast<EPropType>(
						Rng.RandRange(0, static_cast<int32>(EPropType::MAX) - 1));
					Prop->Configure(Type);
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
				SpawnedActors.Add(Monster);
			}
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

	// Walk every floor cell; on edges that border a non-floor cell (a wall) place a torch on a
	// regular cell spacing. Vertical walls (along Y) are spaced by Y; horizontal walls by X.
	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (!IsFloor(x, y))
			{
				continue;
			}

			const FVector C = CellToLocal(x, y);

			if (!IsFloor(x + 1, y) && (y % Spacing) == 0) { PlaceWallTorch(C, FVector(1.f, 0.f, 0.f)); }
			if (!IsFloor(x - 1, y) && (y % Spacing) == 0) { PlaceWallTorch(C, FVector(-1.f, 0.f, 0.f)); }
			if (!IsFloor(x, y + 1) && (x % Spacing) == 0) { PlaceWallTorch(C, FVector(0.f, 1.f, 0.f)); }
			if (!IsFloor(x, y - 1) && (x % Spacing) == 0) { PlaceWallTorch(C, FVector(0.f, -1.f, 0.f)); }
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
	const float TorchZ = WallHeight * 0.55f;

	// Bracket sits flush on the inner wall face; the flame nub sits just in front of and above it.
	const FVector BracketLocal = CellLocal + OutwardDir * (HalfCell - 8.f) + FVector(0.f, 0.f, TorchZ);
	const FVector FlameLocal = BracketLocal - OutwardDir * 12.f + FVector(0.f, 0.f, 18.f);

	AddTile(TorchISM, BracketLocal, FVector(14.f, 14.f, 10.f)); // bracket
	AddTile(TorchISM, FlameLocal, FVector(9.f, 9.f, 14.f));     // flame nub

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
			LC->SetLightColor(FLinearColor(1.0f, 0.74f, 0.42f)); // warm flame
			LC->SetSourceRadius(14.f);
			LC->SetSoftSourceRadius(40.f);
			// Many torches: keep them as cheap, soft fill (no per-light shadows). Lumen + the sky
			// light give depth; this avoids dozens of shadow-casting movable lights.
			LC->SetCastShadows(false);
		}
		SpawnedActors.Add(Light);
	}
}
