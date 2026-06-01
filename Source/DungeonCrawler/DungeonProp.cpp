#include "DungeonProp.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

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

	// Blender swap-in: a single finished mesh replaces the whole graybox assembly.
	if (MeshOverride)
	{
		UStaticMeshComponent* Whole = NewObject<UStaticMeshComponent>(this);
		Whole->SetupAttachment(Root);
		Whole->RegisterComponent();
		Whole->SetMobility(EComponentMobility::Movable);
		Whole->SetStaticMesh(MeshOverride);
		Whole->SetCollisionProfileName(TEXT("BlockAll"));
		Parts.Add(Whole);
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
	case EPropType::Chair:
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
	default:
		break;
	}
}
