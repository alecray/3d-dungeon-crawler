#include "DungeonProp.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

// Content paths that finished props pull their meshes from (import your assets here).
static const TCHAR* StoolMeshPath = TEXT("/Game/Furniture/SM_Stool.SM_Stool");
static const TCHAR* CrateMeshPath = TEXT("/Game/Furniture/SM_Crate.SM_Crate");
static const TCHAR* TableMeshPath = TEXT("/Game/Furniture/SM_Table.SM_Table");

// Imported scenery meshes (no graybox fallback): one mesh per type.
static const TCHAR* MeshPathForType(EPropType Type)
{
	switch (Type)
	{
	case EPropType::Barrel:     return TEXT("/Game/Furniture/SM_Barrel.SM_Barrel");
	case EPropType::Pots:       return TEXT("/Game/Furniture/SM_Pots.SM_Pots");
	case EPropType::Bucket:     return TEXT("/Game/Furniture/SM_Bucket.SM_Bucket");
	case EPropType::Anvil:      return TEXT("/Game/Furniture/SM_Anvil.SM_Anvil");
	case EPropType::Coffin:     return TEXT("/Game/Furniture/SM_Coffin.SM_Coffin");
	case EPropType::Cage:       return TEXT("/Game/Furniture/SM_Cage.SM_Cage");
	case EPropType::WeaponRack: return TEXT("/Game/Furniture/SM_Weapon_Rack.SM_Weapon_Rack");
	case EPropType::Banner:     return TEXT("/Game/Furniture/SM_Banner.SM_Banner");
	case EPropType::Rocks:      return TEXT("/Game/World/rocks.rocks");
	case EPropType::Bones:      return TEXT("/Game/World/bones.bones");
	case EPropType::Mushrooms:  return TEXT("/Game/World/mushroom.mushroom");
	default:                    return nullptr;
	}
}

// The engine cube is a 100cm cube centered on its origin, so a component scale of 1.0 == 100cm.
static constexpr float CubeUnitCm = 100.f;

ADungeonProp::ADungeonProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}
}

void ADungeonProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ADungeonProp::Configure(EPropType InType)
{
	PropType = InType;
	Rebuild();
}

UStaticMeshComponent* ADungeonProp::AddBox(const FVector& Center, const FVector& SizeCm)
{
	UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this);
	Box->SetupAttachment(Root);
	Box->RegisterComponent();
	Box->SetMobility(EComponentMobility::Movable);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	if (CubeMesh)
	{
		Box->SetStaticMesh(CubeMesh);
	}
	Box->SetRelativeLocation(Center);
	Box->SetRelativeScale3D(SizeCm / CubeUnitCm);
	Parts.Add(Box);
	return Box;
}

void ADungeonProp::Rebuild()
{
	// Tear down any previously-built parts (e.g. when PropType is changed in-editor).
	for (UStaticMeshComponent* Part : Parts)
	{
		if (IsValid(Part))
		{
			Part->DestroyComponent();
		}
	}
	Parts.Reset();

	// Finished-mesh swap-in: a single imported mesh replaces the whole graybox assembly.
	if (UStaticMesh* Finished = GetFinishedMesh())
	{
		BuildFinishedMesh(Finished);
		return;
	}

	switch (PropType)
	{
	case EPropType::Table:
	{
		const float TopZ = 75.f;
		const FVector TopSize(120.f, 80.f, 8.f);
		AddBox(FVector(0.f, 0.f, TopZ), TopSize);
		const float LegX = TopSize.X * 0.5f - 10.f;
		const float LegY = TopSize.Y * 0.5f - 10.f;
		const FVector LegSize(8.f, 8.f, TopZ);
		AddBox(FVector(LegX, LegY, TopZ * 0.5f), LegSize);
		AddBox(FVector(-LegX, LegY, TopZ * 0.5f), LegSize);
		AddBox(FVector(LegX, -LegY, TopZ * 0.5f), LegSize);
		AddBox(FVector(-LegX, -LegY, TopZ * 0.5f), LegSize);
		break;
	}
	case EPropType::Stool:
	{
		const float SeatZ = 45.f;
		const FVector SeatSize(45.f, 45.f, 6.f);
		AddBox(FVector(0.f, 0.f, SeatZ), SeatSize);
		const float LegX = SeatSize.X * 0.5f - 5.f;
		const float LegY = SeatSize.Y * 0.5f - 5.f;
		const FVector LegSize(6.f, 6.f, SeatZ);
		AddBox(FVector(LegX, LegY, SeatZ * 0.5f), LegSize);
		AddBox(FVector(-LegX, LegY, SeatZ * 0.5f), LegSize);
		AddBox(FVector(LegX, -LegY, SeatZ * 0.5f), LegSize);
		AddBox(FVector(-LegX, -LegY, SeatZ * 0.5f), LegSize);
		// Backrest along -X.
		AddBox(FVector(-LegX, 0.f, SeatZ + 22.f), FVector(6.f, SeatSize.Y, 45.f));
		break;
	}
	case EPropType::Cabinet:
	{
		const FVector Size(60.f, 40.f, 140.f);
		AddBox(FVector(0.f, 0.f, Size.Z * 0.5f), Size);
		break;
	}
	case EPropType::Dresser:
	{
		const FVector Size(90.f, 45.f, 80.f);
		AddBox(FVector(0.f, 0.f, Size.Z * 0.5f), Size);
		// Drawer seams for readability.
		AddBox(FVector(Size.X * 0.5f + 1.f, 0.f, Size.Z * 0.66f), FVector(3.f, Size.Y * 0.7f, 4.f));
		AddBox(FVector(Size.X * 0.5f + 1.f, 0.f, Size.Z * 0.33f), FVector(3.f, Size.Y * 0.7f, 4.f));
		break;
	}
	case EPropType::Bookshelf:
	{
		const FVector Frame(90.f, 30.f, 180.f);
		// Back panel + two sides instead of a solid block, so it reads as a shelf.
		AddBox(FVector(-Frame.X * 0.5f + 2.f, 0.f, Frame.Z * 0.5f), FVector(4.f, Frame.Y, Frame.Z));
		AddBox(FVector(0.f, Frame.Y * 0.5f - 2.f, Frame.Z * 0.5f), FVector(Frame.X, 4.f, Frame.Z));
		AddBox(FVector(0.f, -Frame.Y * 0.5f + 2.f, Frame.Z * 0.5f), FVector(Frame.X, 4.f, Frame.Z));
		// Horizontal shelves.
		const int32 NumShelves = 4;
		for (int32 i = 0; i <= NumShelves; ++i)
		{
			const float Z = (Frame.Z / NumShelves) * i;
			AddBox(FVector(0.f, 0.f, Z), FVector(Frame.X, Frame.Y, 4.f));
		}
		break;
	}
	case EPropType::Crate:
	{
		// Simple box graybox (normally replaced by the imported SM_Crate mesh).
		const FVector Size(70.f, 70.f, 70.f);
		AddBox(FVector(0.f, 0.f, Size.Z * 0.5f), Size);
		break;
	}
	default:
		break;
	}
}

UStaticMesh* ADungeonProp::GetFinishedMesh() const
{
	// A per-instance override always wins.
	if (MeshOverride)
	{
		return MeshOverride;
	}

	// Otherwise use the imported mesh registered for this prop type (only Stool for now). An
	// editor-assigned StoolMesh wins; failing that, try to load it from the conventional path at
	// runtime so importing the stool and re-Playing is enough (no editor restart needed).
	switch (PropType)
	{
	case EPropType::Stool:
		if (StoolMesh)
		{
			return StoolMesh;
		}
		return Cast<UStaticMesh>(FSoftObjectPath(StoolMeshPath).TryLoad());
	case EPropType::Crate:
		if (CrateMesh)
		{
			return CrateMesh;
		}
		return Cast<UStaticMesh>(FSoftObjectPath(CrateMeshPath).TryLoad());
	case EPropType::Table:
		if (TableMesh)
		{
			return TableMesh;
		}
		return Cast<UStaticMesh>(FSoftObjectPath(TableMeshPath).TryLoad());
	default:
		// Imported scenery meshes (barrel/pots/rocks/etc.): load from the per-type path.
		if (const TCHAR* Path = MeshPathForType(PropType))
		{
			return Cast<UStaticMesh>(FSoftObjectPath(Path).TryLoad());
		}
		return nullptr;
	}
}

void ADungeonProp::BuildFinishedMesh(UStaticMesh* Mesh)
{
	UStaticMeshComponent* Whole = NewObject<UStaticMeshComponent>(this);
	Whole->SetupAttachment(Root);
	Whole->RegisterComponent();
	Whole->SetMobility(EComponentMobility::Movable);
	Whole->SetStaticMesh(Mesh);
	Whole->SetCollisionProfileName(TEXT("BlockAll"));
	Parts.Add(Whole);
}
