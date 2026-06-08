#include "DayNightCycle.h"

#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ADayNightCycle::ADayNightCycle()
{
	PrimaryActorTick.bCanEverTick = true;

	// The star dome is the root so the whole actor centers it on the town.
	StarDome = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarDome"));
	SetRootComponent(StarDome);
	StarDome->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarDome->SetCastShadow(false);
	StarDome->bCastDynamicShadow = false;
	StarDome->SetRelativeScale3D(FVector(StarDomeScale));
}

void ADayNightCycle::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Find the placed town sun (first directional light) and flip it to Movable so it can rotate at
	// runtime — done in code so the saved L_Town scene / build_town.py are untouched.
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			SunComp = DLC;
			SunComp->SetMobility(EComponentMobility::Movable);
			break;
		}
	}

	// Find the (real-time-capture) sky light and the height fog so we can drive the ambient floor + fog tint.
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		SkyLightComp = It->GetLightComponent();
		break;
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		FogComp = It->GetComponent();
		break;
	}

	// Spawn our own moon: a second atmosphere sun light (index 1) so the sky scatters a night sky for it.
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MoonLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),
		FTransform(FRotator(45.f, SunYaw + 180.f, 0.f), GetActorLocation()), Params);
	if (MoonLight)
	{
		if (UDirectionalLightComponent* M = Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent()))
		{
			M->SetMobility(EComponentMobility::Movable);
			M->SetLightColor(MoonColor);
			M->SetIntensity(0.f);
			M->bAtmosphereSunLight = true; // a second "sun" for the atmosphere = the moon
			M->AtmosphereSunLightIndex = 1;
			M->MarkRenderStateDirty();
		}
	}

	// Set up the star dome mesh + a dynamic instance of the additive star material we can fade in at night.
	if (StarDome)
	{
		if (UStaticMesh* DomeMesh = StarDomeMesh.LoadSynchronous())
		{
			StarDome->SetStaticMesh(DomeMesh);
		}
		StarDome->SetWorldScale3D(FVector(StarDomeScale));
		if (UMaterialInterface* StarMat = StarMaterial.LoadSynchronous())
		{
			StarMID = UMaterialInstanceDynamic::Create(StarMat, this);
			if (StarMID)
			{
				StarDome->SetMaterial(0, StarMID);
			}
		}
		// Null mesh/material (e.g. M_Stars not authored yet) just means no stars — the cycle still runs.
	}

	TimeOfDay = StartTimeOfDay;
	Apply(TimeOfDay);
}

void ADayNightCycle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DayLengthSeconds > 0.f)
	{
		TimeOfDay += DeltaSeconds / DayLengthSeconds;
		TimeOfDay = FMath::Frac(TimeOfDay); // wrap to [0,1)
	}
	Apply(TimeOfDay);
}

void ADayNightCycle::Apply(float Time01)
{
	// Sun altitude over the day: -1 at midnight, 0 at sunrise/sunset, +1 at noon.
	const float Altitude = FMath::Sin((Time01 - 0.25f) * 2.f * PI);
	const float DayFactor = FMath::Clamp(Altitude, 0.f, 1.f);    // 0 at/below horizon, 1 overhead
	const float NightFactor = FMath::Clamp(-Altitude, 0.f, 1.f); // inverse, for the moon/stars

	// Sun: rotate so it points down when overhead; fade out + warm toward the horizon.
	if (SunComp)
	{
		SunComp->SetWorldRotation(FRotator(-Altitude * 90.f, SunYaw, 0.f));
		SunComp->SetIntensity(SunPeakIntensity * DayFactor);
		SunComp->SetLightColor(FLinearColor::LerpUsingHSV(SunHorizonColor, SunNoonColor, DayFactor));
	}

	// Moon: opposite the sun, brightening at night.
	if (MoonLight)
	{
		if (UDirectionalLightComponent* M = Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent()))
		{
			M->SetWorldRotation(FRotator(Altitude * 90.f, SunYaw + 180.f, 0.f));
			M->SetIntensity(MoonPeakIntensity * NightFactor);
		}
	}

	// Ambient floor: keep some sky light at night so the town stays readable.
	if (SkyLightComp)
	{
		SkyLightComp->SetIntensity(FMath::Lerp(MinSkyLightIntensity, MaxSkyLightIntensity, DayFactor));
	}

	// Fog tint warms by day, deep blue by night.
	if (FogComp)
	{
		FogComp->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(FogNightColor, FogDayColor, DayFactor));
	}

	// Stars: only in deep night, fully gone before dawn/dusk. A plain linear fade left the (high-frequency)
	// stars rendering at low opacity over a brightening sky at dawn, which read as shimmer/static. SmoothStep
	// from a threshold keeps them at 0 through the transition, and we hide the dome entirely once dark, so the
	// noisy material isn't drawn at all outside deep night.
	const float StarFactor = FMath::SmoothStep(StarNightThreshold, FMath::Min(1.f, StarNightThreshold + 0.45f), NightFactor);
	const float StarBrightness = StarMaxBrightness * StarFactor;
	if (StarMID)
	{
		StarMID->SetScalarParameterValue(TEXT("StarBrightness"), StarBrightness);
	}
	if (StarDome)
	{
		StarDome->SetVisibility(StarBrightness > KINDA_SMALL_NUMBER);
	}
}
