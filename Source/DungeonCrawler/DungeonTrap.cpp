#include "DungeonTrap.h"
#include "HealthComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

// Conventional content paths for finished trap meshes (import here to replace the graybox).
static const TCHAR* SpikeMeshPath = TEXT("/Game/Traps/SM_Spikes.SM_Spikes");
static const TCHAR* DartLauncherMeshPath = TEXT("/Game/Traps/SM_Dart_Launcher.SM_Dart_Launcher");
static const TCHAR* DartMeshPath = TEXT("/Game/Traps/SM_Dart.SM_Dart");
static const TCHAR* PlateMeshPath = TEXT("/Game/Traps/SM_Pressure_Plate.SM_Pressure_Plate");

// The engine cube is a 100cm cube centered on its origin, so a component scale of 1.0 == 100cm.
static constexpr float TrapCubeUnitCm = 100.f;

// Graybox dimensions (cm).
static constexpr float SpikeTravel = 70.f;   // how far the spikes rise out of the floor
static constexpr float PlateDrop   = 8.f;    // how far the pressure plate sinks when stepped on
static constexpr float TrapFootprint = 220.f; // XY span of the spike/plate hazard
static constexpr float MuzzleZ = 150.f;       // dart launch height above the floor

ADungeonTrap::ADungeonTrap()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}
}

void ADungeonTrap::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ADungeonTrap::Configure(ETrapType InType)
{
	TrapType = InType;
	Rebuild();
}

void ADungeonTrap::BeginPlay()
{
	Super::BeginPlay();

	// Desync identical traps so they don't all cycle/fire in lockstep.
	FireTimer = FMath::FRandRange(0.f, DartInterval);
	StateTimer = FMath::FRandRange(0.f, FMath::Max(0.01f, SpikeDownTime));
	State = EState::Idle;
}

UStaticMesh* ADungeonTrap::ResolveMesh(UStaticMesh* Override, const TCHAR* SoftPath) const
{
	if (Override)
	{
		return Override;
	}
	return Cast<UStaticMesh>(FSoftObjectPath(SoftPath).TryLoad());
}

UStaticMeshComponent* ADungeonTrap::AddBox(USceneComponent* Parent, const FVector& Center, const FVector& SizeCm, bool bCollide)
{
	UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this);
	USceneComponent* AttachTo = Parent ? Parent : Root.Get();
	Box->SetupAttachment(AttachTo);
	Box->RegisterComponent();
	Box->SetMobility(EComponentMobility::Movable);
	if (bCollide)
	{
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (CubeMesh)
	{
		Box->SetStaticMesh(CubeMesh);
	}
	Box->SetRelativeLocation(Center);
	Box->SetRelativeScale3D(SizeCm / TrapCubeUnitCm);
	Parts.Add(Box);
	return Box;
}

void ADungeonTrap::Rebuild()
{
	// Tear down anything from a previous build (type changed in-editor, or Configure after construction).
	for (UStaticMeshComponent* Part : Parts)
	{
		if (IsValid(Part)) { Part->DestroyComponent(); }
	}
	Parts.Reset();
	for (const FTracer& T : Tracers)
	{
		if (IsValid(T.Comp)) { T.Comp->DestroyComponent(); }
	}
	Tracers.Reset();
	if (IsValid(SpikeGroup)) { SpikeGroup->DestroyComponent(); SpikeGroup = nullptr; }
	if (IsValid(Plate))      { Plate->DestroyComponent();      Plate = nullptr; }
	if (IsValid(Muzzle))     { Muzzle->DestroyComponent();     Muzzle = nullptr; }
	if (IsValid(Trigger))    { Trigger->DestroyComponent();    Trigger = nullptr; }

	switch (TrapType)
	{
	case ETrapType::DartShooter:
		BuildDartShooter();
		break;
	case ETrapType::SpikeFloor:
	case ETrapType::PressurePlate:
	default:
		BuildSpikeTrap();
		break;
	}
}

void ADungeonTrap::BuildSpikeTrap()
{
	const bool bPlate = (TrapType == ETrapType::PressurePlate);

	// A thin recessed base frame around the rim so the hole the spikes come from reads as deliberate.
	if (UStaticMesh* BaseMesh = ResolveMesh(BaseMeshOverride, bPlate ? PlateMeshPath : SpikeMeshPath))
	{
		// Finished base mesh sits flush on the floor (collision off so it never blocks movement).
		UStaticMeshComponent* Whole = AddBox(Root, FVector::ZeroVector, FVector(TrapCubeUnitCm), /*bCollide*/ false);
		Whole->SetStaticMesh(BaseMesh);
		Whole->SetRelativeScale3D(FVector::OneVector);
	}
	else
	{
		// Graybox rim: four low bars framing the footprint.
		const float Half = TrapFootprint * 0.5f;
		const float Bar = 18.f;
		AddBox(Root, FVector( Half, 0.f, 5.f), FVector(Bar, TrapFootprint, 10.f), false);
		AddBox(Root, FVector(-Half, 0.f, 5.f), FVector(Bar, TrapFootprint, 10.f), false);
		AddBox(Root, FVector(0.f,  Half, 5.f), FVector(TrapFootprint, Bar, 10.f), false);
		AddBox(Root, FVector(0.f, -Half, 5.f), FVector(TrapFootprint, Bar, 10.f), false);
	}

	// Pressure plate: a square slab that visibly depresses when stepped on.
	if (bPlate)
	{
		Plate = NewObject<USceneComponent>(this, TEXT("Plate"));
		Plate->SetupAttachment(Root);
		Plate->RegisterComponent();
		const float Inset = TrapFootprint - 50.f;
		AddBox(Plate, FVector(0.f, 0.f, 7.f), FVector(Inset, Inset, 8.f), false);
	}

	// Spikes ride on a group we slide up/down. Built pointing up, hidden below the floor at rest.
	SpikeGroup = NewObject<USceneComponent>(this, TEXT("SpikeGroup"));
	SpikeGroup->SetupAttachment(Root);
	SpikeGroup->RegisterComponent();

	if (UStaticMesh* SpikeMesh = ResolveMesh(SpikeMeshOverride, SpikeMeshPath))
	{
		UStaticMeshComponent* Whole = AddBox(SpikeGroup, FVector(0.f, 0.f, SpikeTravel * 0.5f), FVector(TrapCubeUnitCm), false);
		Whole->SetStaticMesh(SpikeMesh);
		Whole->SetRelativeScale3D(FVector::OneVector);
	}
	else
	{
		// Graybox: a 3x3 grid of tapered spikes (a thin shaft capped by a smaller tip).
		const int32 N = 3;
		const float Step = (TrapFootprint - 40.f) / (N - 1);
		const float Start = -(TrapFootprint - 40.f) * 0.5f;
		for (int32 ix = 0; ix < N; ++ix)
		{
			for (int32 iy = 0; iy < N; ++iy)
			{
				const float X = Start + ix * Step;
				const float Y = Start + iy * Step;
				AddBox(SpikeGroup, FVector(X, Y, SpikeTravel * 0.5f), FVector(16.f, 16.f, SpikeTravel), false);
				AddBox(SpikeGroup, FVector(X, Y, SpikeTravel + 8.f), FVector(7.f, 7.f, 16.f), false);
			}
		}
	}

	SetSpikeExtension(0.f); // start retracted

	// Hazard footprint: a box standing on the cell that detects pawns to damage when spikes are up.
	Trigger = NewObject<UBoxComponent>(this, TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->RegisterComponent();
	Trigger->SetBoxExtent(FVector(TrapFootprint * 0.5f, TrapFootprint * 0.5f, 90.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ADungeonTrap::BuildDartShooter()
{
	// Launcher body mounted flush against the wall, facing +X (its local forward) into the open space.
	if (UStaticMesh* BodyMesh = ResolveMesh(BaseMeshOverride, DartLauncherMeshPath))
	{
		UStaticMeshComponent* Whole = AddBox(Root, FVector(0.f, 0.f, MuzzleZ), FVector(TrapCubeUnitCm), /*bCollide*/ true);
		Whole->SetStaticMesh(BodyMesh);
		Whole->SetRelativeScale3D(FVector::OneVector);
	}
	else
	{
		// Graybox: a chunky block with a darker barrel and a small bore facing forward.
		AddBox(Root, FVector(-6.f, 0.f, MuzzleZ), FVector(28.f, 60.f, 60.f), true);
		AddBox(Root, FVector(14.f, 0.f, MuzzleZ), FVector(24.f, 22.f, 22.f), true);
	}

	Muzzle = NewObject<USceneComponent>(this, TEXT("Muzzle"));
	Muzzle->SetupAttachment(Root);
	Muzzle->RegisterComponent();
	Muzzle->SetRelativeLocation(FVector(28.f, 0.f, MuzzleZ));
}

void ADungeonTrap::SetSpikeExtension(float Alpha)
{
	if (SpikeGroup)
	{
		Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
		// Alpha 0 = tips hidden below the floor; Alpha 1 = spikes fully up.
		SpikeGroup->SetRelativeLocation(FVector(0.f, 0.f, -SpikeTravel - 24.f + Alpha * (SpikeTravel + 24.f)));
	}
}

void ADungeonTrap::SetPlateDepressed(bool bDown)
{
	if (Plate)
	{
		Plate->SetRelativeLocation(FVector(0.f, 0.f, bDown ? -PlateDrop : 0.f));
	}
}

void ADungeonTrap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DamageTimer > 0.f) { DamageTimer -= DeltaSeconds; }

	switch (TrapType)
	{
	case ETrapType::SpikeFloor:    TickSpikeFloor(DeltaSeconds); break;
	case ETrapType::PressurePlate: TickPressurePlate(DeltaSeconds); break;
	case ETrapType::DartShooter:   TickDartShooter(DeltaSeconds); break;
	default: break;
	}

	TickTracers(DeltaSeconds);
}

void ADungeonTrap::TickSpikeFloor(float Dt)
{
	StateTimer += Dt;
	switch (State)
	{
	case EState::Idle: // retracted, waiting
		if (StateTimer >= SpikeDownTime) { State = EState::Rising; StateTimer = 0.f; }
		break;
	case EState::Rising:
		SetSpikeExtension(StateTimer / SpikeMoveTime);
		if (StateTimer >= SpikeMoveTime) { SetSpikeExtension(1.f); State = EState::Raised; StateTimer = 0.f; DamageTimer = 0.f; }
		break;
	case EState::Raised:
		DamageOverlappers();
		if (StateTimer >= SpikeUpTime) { State = EState::Falling; StateTimer = 0.f; }
		break;
	case EState::Falling:
		SetSpikeExtension(1.f - StateTimer / SpikeMoveTime);
		if (StateTimer >= SpikeMoveTime) { SetSpikeExtension(0.f); State = EState::Idle; StateTimer = 0.f; }
		break;
	default:
		State = EState::Idle; StateTimer = 0.f;
		break;
	}
}

void ADungeonTrap::TickPressurePlate(float Dt)
{
	StateTimer += Dt;
	switch (State)
	{
	case EState::Idle: // armed: plate up, spikes hidden — wait for a step
	{
		TArray<AActor*> Overlap;
		if (Trigger) { Trigger->GetOverlappingActors(Overlap, APawn::StaticClass()); }
		if (Overlap.Num() > 0)
		{
			SetPlateDepressed(true);
			State = EState::Telegraph;
			StateTimer = 0.f;
		}
		break;
	}
	case EState::Telegraph:
		if (StateTimer >= PlateTelegraph) { State = EState::Rising; StateTimer = 0.f; }
		break;
	case EState::Rising:
		SetSpikeExtension(StateTimer / SpikeMoveTime);
		if (StateTimer >= SpikeMoveTime) { SetSpikeExtension(1.f); State = EState::Raised; StateTimer = 0.f; DamageTimer = 0.f; }
		break;
	case EState::Raised:
		DamageOverlappers();
		if (StateTimer >= SpikeUpTime) { State = EState::Falling; StateTimer = 0.f; }
		break;
	case EState::Falling:
		SetSpikeExtension(1.f - StateTimer / SpikeMoveTime);
		if (StateTimer >= SpikeMoveTime) { SetSpikeExtension(0.f); SetPlateDepressed(false); State = EState::Cooldown; StateTimer = 0.f; }
		break;
	case EState::Cooldown:
		if (StateTimer >= PlateRearmTime) { State = EState::Idle; StateTimer = 0.f; }
		break;
	default:
		State = EState::Idle; StateTimer = 0.f;
		break;
	}
}

void ADungeonTrap::TickDartShooter(float Dt)
{
	FireTimer -= Dt;
	if (FireTimer <= 0.f)
	{
		FireDart();
		FireTimer = DartInterval;
	}
}

void ADungeonTrap::DamageOverlappers()
{
	if (DamageTimer > 0.f || !Trigger)
	{
		return;
	}

	TArray<AActor*> Overlap;
	Trigger->GetOverlappingActors(Overlap, APawn::StaticClass());
	bool bHitSomething = false;
	for (AActor* Actor : Overlap)
	{
		if (UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr)
		{
			if (!Health->IsDead())
			{
				Health->ApplyDamage(Damage);
				bHitSomething = true;
			}
		}
	}
	if (bHitSomething)
	{
		DamageTimer = DamageInterval;
	}
}

void ADungeonTrap::FireDart()
{
	UWorld* World = GetWorld();
	if (!World || !Muzzle)
	{
		return;
	}

	const FVector Start = Muzzle->GetComponentLocation();
	const FVector Dir = GetActorForwardVector();
	const FVector End = Start + Dir * DartRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DungeonDart), /*bTraceComplex*/ false, this);
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);
	const FVector Impact = bHit ? Hit.ImpactPoint : End;

	if (bHit && Hit.GetActor())
	{
		if (UHealthComponent* Health = Hit.GetActor()->FindComponentByClass<UHealthComponent>())
		{
			if (!Health->IsDead())
			{
				Health->ApplyDamage(Damage);
			}
		}
	}

	// Spawn a short-lived graybox tracer streak from muzzle to impact (swap DartMeshOverride for FBX).
	const float Length = FMath::Max(1.f, FVector::Dist(Start, Impact));
	const FVector Mid = (Start + Impact) * 0.5f;

	UStaticMeshComponent* Streak = NewObject<UStaticMeshComponent>(this);
	Streak->SetupAttachment(Root);
	Streak->RegisterComponent();
	Streak->SetMobility(EComponentMobility::Movable);
	Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* DartMesh = ResolveMesh(DartMeshOverride, DartMeshPath))
	{
		Streak->SetStaticMesh(DartMesh);
		Streak->SetWorldLocation(Impact);
		Streak->SetWorldRotation(Dir.Rotation());
		Streak->SetWorldScale3D(FVector::OneVector);
	}
	else if (CubeMesh)
	{
		Streak->SetStaticMesh(CubeMesh);
		Streak->SetWorldLocation(Mid);
		Streak->SetWorldRotation(Dir.Rotation());
		// Long thin shaft along local X.
		Streak->SetWorldScale3D(FVector(Length / TrapCubeUnitCm, 0.05f, 0.05f));
	}
	Tracers.Add({ Streak, 0.09f });
}

void ADungeonTrap::TickTracers(float Dt)
{
	for (int32 i = Tracers.Num() - 1; i >= 0; --i)
	{
		Tracers[i].Life -= Dt;
		if (Tracers[i].Life <= 0.f)
		{
			if (IsValid(Tracers[i].Comp)) { Tracers[i].Comp->DestroyComponent(); }
			Tracers.RemoveAtSwap(i);
		}
	}
}
