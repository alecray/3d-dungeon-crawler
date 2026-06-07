#include "BossDoor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

// The engine cube is a 100cm cube centered on its origin, so a component scale of 1.0 == 100cm.
static constexpr float DoorCubeUnitCm = 100.f;
static constexpr float RaiseSpeed = 3.5f; // alpha per second (≈0.3s to fully raise/lower)

ABossDoor::ABossDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}
}

void ABossDoor::Init(const FVector& SizeCm)
{
	Height = FMath::Max(50.f, SizeCm.Z);
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(SizeCm / DoorCubeUnitCm);
	}
	ApplyOffset(); // start hidden below the floor
}

void ABossDoor::Seal()
{
	Target = 1.f;
	bOpening = false;
}

void ABossDoor::Open()
{
	Target = 0.f;
	bOpening = true;
}

void ABossDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Current == Target)
	{
		if (bOpening && Current <= 0.f)
		{
			Destroy();
		}
		return;
	}

	Current = FMath::FInterpConstantTo(Current, Target, DeltaSeconds, RaiseSpeed);
	ApplyOffset();
}

void ABossDoor::ApplyOffset()
{
	if (Mesh)
	{
		// Cube center sits at +Height/2 when raised (its base at the floor); fully hidden one height below.
		const float CenterZ = Height * 0.5f - (1.f - Current) * Height;
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
	}
}
