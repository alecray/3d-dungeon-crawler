#include "Portal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPath.h"

static const FLinearColor PortalColor(0.25f, 0.85f, 1.0f); // cyan energy

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UStaticMesh* Cylinder = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad());
	UStaticMesh* Cube = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());

	// Portal surface: a thin disc (flattened cylinder) stood vertical so it faces along X. Blocks only
	// the Visibility channel so the interact line-trace can target it without impeding movement.
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(Root);
	Disc->SetStaticMesh(Cylinder);
	Disc->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
	Disc->SetRelativeRotation(FRotator(90.f, 0.f, 0.f)); // lay the cylinder axis along X -> vertical disc
	Disc->SetRelativeScale3D(FVector(1.5f, 1.5f, 0.06f)); // wide radius, very thin
	Disc->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Disc->SetCollisionResponseToAllChannels(ECR_Ignore);
	Disc->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Spinning ring of shards around the disc rim (in the YZ plane, since the disc faces X).
	Spinner = CreateDefaultSubobject<USceneComponent>(TEXT("Spinner"));
	Spinner->SetupAttachment(Root);
	Spinner->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));

	for (int32 i = 0; i < ShardCount; ++i)
	{
		const float Angle = (2.f * PI * i) / ShardCount;
		UStaticMeshComponent* Shard = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Shard%d"), i));
		Shard->SetupAttachment(Spinner);
		Shard->SetStaticMesh(Cube);
		Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shard->SetRelativeLocation(FVector(0.f, RingRadius * FMath::Cos(Angle), RingRadius * FMath::Sin(Angle)));
		Shard->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(Angle), 0.f, 0.f)); // point radially
		Shard->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.3f));
		Shards.Add(Shard);
	}

	// Bright cyan glow centered in the gate.
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
	Glow->SetLightColor(PortalColor);
	Glow->SetIntensity(BaseIntensity);
	Glow->SetAttenuationRadius(900.f);
	Glow->SetCastShadows(false);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();
	ApplyGlowMaterial(Disc);
	for (UStaticMeshComponent* Shard : Shards)
	{
		ApplyGlowMaterial(Shard);
	}
	SetActive(bActive);
}

void APortal::ApplyGlowMaterial(UStaticMeshComponent* Comp)
{
	if (!Comp)
	{
		return;
	}
	// Tint the basic-shape material cyan; with the bright local light this reads as glowing energy.
	if (UMaterialInterface* Base = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")).TryLoad()))
	{
		if (UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0, Base))
		{
			MID->SetVectorParameterValue(TEXT("Color"), PortalColor);
		}
	}
}

void APortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}

	Elapsed += DeltaSeconds;

	// Spin the ring around the disc axis (X), and gently pulse the light.
	if (Spinner)
	{
		Spinner->SetRelativeRotation(FRotator(0.f, 0.f, Elapsed * 70.f)); // roll spins the YZ-plane ring
	}
	if (Glow)
	{
		Glow->SetIntensity(BaseIntensity * (0.8f + 0.2f * FMath::Sin(Elapsed * 3.f)));
	}
}

void APortal::SetActive(bool bInActive)
{
	bActive = bInActive;
	// Hidden + non-collidable when dormant, so it can't be seen or used until switched on.
	SetActorHiddenInGame(!bActive);
	if (Disc)
	{
		Disc->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (Glow)
	{
		Glow->SetVisibility(bActive);
	}
}
