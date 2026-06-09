#include "TownTorch.h"
#include "DayNightCycle.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPath.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ATownTorch::ATownTorch()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UStaticMesh* TorchSM = Cast<UStaticMesh>(
		FSoftObjectPath(TEXT("/Game/Furniture/standing_torch.standing_torch")).TryLoad());
	UStaticMesh* Cube = Cast<UStaticMesh>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());

	TorchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchMesh"));
	TorchMesh->SetupAttachment(Root);
	if (TorchSM) { TorchMesh->SetStaticMesh(TorchSM); }
	TorchMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Small flame cube above the torch bowl; position is corrected in BeginPlay from FlameHeight.
	Flame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Root);
	if (Cube) { Flame->SetStaticMesh(Cube); }
	Flame->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.18f));
	Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetLightColor(FLinearColor(1.0f, 0.55f, 0.18f));
	Light->SetIntensity(0.f); // off until BeginPlay applies night factor
	Light->SetAttenuationRadius(700.f);
	Light->SetCastShadows(false);
}

void ATownTorch::BeginPlay()
{
	Super::BeginPlay();

	// Reposition flame and light to the editable FlameHeight.
	Flame->SetRelativeLocation(FVector(0.f, 0.f, FlameHeight));
	Light->SetRelativeLocation(FVector(0.f, 0.f, FlameHeight + 10.f));

	// Apply the torch material to the mesh if the project material is available.
	if (UMaterialInterface* MI = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Furniture/MI_Standing_Torch.MI_Standing_Torch")).TryLoad()))
	{
		TorchMesh->SetMaterial(0, MI);
	}

	// Flame emissive material — driven each tick via FlameMID.
	if (UMaterialInterface* Base = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")).TryLoad()))
	{
		FlameMID = Flame->CreateDynamicMaterialInstance(0, Base);
	}

	// Cache the town's day/night cycle so Tick can read the time of day cheaply.
	for (TActorIterator<ADayNightCycle> It(GetWorld()); It; ++It)
	{
		DayCycle = *It;
		break;
	}
}

void ATownTorch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Compute how "night" it is using the same altitude formula as ADayNightCycle.
	// NightBrightness = 1 at midnight, 0 at noon, smooth ramp through dusk/dawn.
	float NightBrightness = 1.f;
	if (DayCycle)
	{
		const float T        = DayCycle->GetTimeOfDay();
		const float Altitude = FMath::Sin((T - 0.25f) * 2.f * PI);
		const float DayFactor = FMath::Clamp(Altitude, 0.f, 1.f);
		NightBrightness = 1.f - DayFactor;
	}

	// Organic flicker — two overlapping sines, same recipe as ABonfire.
	FlickerPhase += DeltaSeconds * 11.f;
	const float Flicker = 0.82f + 0.18f * FMath::Sin(FlickerPhase) * FMath::Sin(FlickerPhase * 2.3f + 1.f);

	if (Light)
	{
		Light->SetIntensity(BaseLightIntensity * NightBrightness * Flicker);
	}

	if (FlameMID)
	{
		FlameMID->SetVectorParameterValue(TEXT("Color"), FlameColor * NightBrightness * Flicker);
	}
}
