#include "DungeonCrawlerGameMode.h"
#include "DungeonGenerator.h"
#include "FirstPersonCharacter.h"
#include "DungeonPlayerController.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

ADungeonCrawlerGameMode::ADungeonCrawlerGameMode()
{
	DefaultPawnClass = AFirstPersonCharacter::StaticClass();
	PlayerControllerClass = ADungeonPlayerController::StaticClass();
	DungeonGeneratorClass = ADungeonGenerator::StaticClass();
}

void ADungeonCrawlerGameMode::BeginPlay()
{
	Super::BeginPlay();
	BuildWorld();
}

void ADungeonCrawlerGameMode::EnsureLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Only add lights the level is missing (an empty level has none; a template ships its own).
	bool bHasDirectional = false;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It) { bHasDirectional = true; break; }

	bool bHasSky = false;
	for (TActorIterator<ASkyLight> It(World); It; ++It) { bHasSky = true; break; }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!bHasDirectional)
	{
		// Soft warm "moonlight" fill with a wide source angle for gentle penumbra shadows.
		if (ADirectionalLight* Key = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(), FTransform(FRotator(-50.f, -40.f, 0.f), FVector::ZeroVector), Params))
		{
			if (ULightComponent* L = Key->GetLightComponent())
			{
				L->SetMobility(EComponentMobility::Movable);
				L->SetIntensity(1.0f);
				L->SetLightColor(FLinearColor(0.85f, 0.88f, 1.0f)); // cool fill
				if (UDirectionalLightComponent* DL = Cast<UDirectionalLightComponent>(L))
				{
					DL->SetLightSourceAngle(3.0f); // softer sun shadows
				}
			}
		}
	}

	if (!bHasSky)
	{
		// Strong ambient so shadowed areas read as soft shade rather than black. Lumen uses this
		// plus the point lights for gentle indirect bounce.
		if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(), FTransform(FVector::ZeroVector), Params))
		{
			if (USkyLightComponent* SC = Sky->GetLightComponent())
			{
				SC->SetMobility(EComponentMobility::Movable);
				SC->SetIntensity(3.0f);                              // strong, even ambient fill
				SC->SetLightColor(FLinearColor(0.7f, 0.72f, 0.82f)); // soft, near-neutral
				SC->bLowerHemisphereIsBlack = false;                 // fill the floor/shadows too
				SC->LowerHemisphereColor = FLinearColor(0.4f, 0.4f, 0.45f);
				SC->RecaptureSky();
			}
		}
	}
}

void ADungeonCrawlerGameMode::EnsurePostProcess()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Reuse an existing unbound volume if the level already has one.
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		if (It->bUnbound)
		{
			return;
		}
	}

	APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>();
	if (!PPV)
	{
		return;
	}
	PPV->bUnbound = true; // affects the whole level

	FPostProcessSettings& S = PPV->Settings;

	// Lock exposure (no harsh auto-adjust) and bias it BRIGHT — low-poly games read as well-lit and
	// flat, not moody. A lower locked brightness target makes the rendered image brighter.
	S.bOverride_AutoExposureMinBrightness = true;
	S.AutoExposureMinBrightness = 0.3f;
	S.bOverride_AutoExposureMaxBrightness = true;
	S.AutoExposureMaxBrightness = 0.3f;

	// Ease ambient occlusion so contact shadows aren't a hard black line.
	S.bOverride_AmbientOcclusionIntensity = true;
	S.AmbientOcclusionIntensity = 0.25f;
	S.bOverride_AmbientOcclusionRadius = true;
	S.AmbientOcclusionRadius = 80.f;

	// Strong indirect bounce so shadowed areas fill with soft colored light, never black.
	S.bOverride_IndirectLightingIntensity = true;
	S.IndirectLightingIntensity = 2.2f;

	// Stylized grade: warm white balance, lower contrast, lifted (warm) shadows, a touch more
	// saturation, gentle bloom + vignette — the flat, cozy low-poly look.
	S.bOverride_WhiteTemp = true;
	S.WhiteTemp = 5800.f; // warmer than neutral 6500

	S.bOverride_ColorContrast = true;
	S.ColorContrast = FVector4(0.9f, 0.9f, 0.9f, 1.0f);

	S.bOverride_ColorSaturation = true;
	S.ColorSaturation = FVector4(1.15f, 1.15f, 1.15f, 1.0f);

	S.bOverride_ColorOffset = true;
	S.ColorOffset = FVector4(0.02f, 0.018f, 0.012f, 0.0f); // lift blacks toward warm gray

	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.5f;

	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.3f;
}

void ADungeonCrawlerGameMode::EnsureFog()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Configure an existing fog actor if the level already has one (level templates ship a near-invisible
	// ExponentialHeightFog) — otherwise our settings would be ignored. Spawn one only if none exists.
	UExponentialHeightFogComponent* C = nullptr;
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		C = It->GetComponent();
		break;
	}
	if (!C)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// Fog origin slightly below the floor so the haze fills the rooms and corridors.
		if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(), FTransform(FVector(0.f, 0.f, -50.f)), Params))
		{
			C = Fog->GetComponent();
		}
	}

	if (C)
	{
		// TEMP debug values: very thick + bright so the fog is unmistakable. Dial back once confirmed.
		C->SetFogDensity(1.0f);
		C->SetFogHeightFalloff(0.05f);                                 // barely falls off with height
		C->SetFogInscatteringColor(FLinearColor(0.45f, 0.5f, 0.6f));   // bright cool grey
		C->SetFogMaxOpacity(1.0f);
		C->SetStartDistance(0.f);

		C->SetVolumetricFog(true);
		C->SetVolumetricFogScatteringDistribution(0.2f);
		C->SetVolumetricFogAlbedo(FColor(200, 195, 185));
		C->SetVolumetricFogExtinctionScale(1.0f);
		C->SetVolumetricFogDistance(2500.f);
	}
}

void ADungeonCrawlerGameMode::BuildWorld()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	EnsureLighting();
	EnsurePostProcess();
	EnsureFog();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<ADungeonGenerator> GenClass = DungeonGeneratorClass;
	if (!GenClass)
	{
		GenClass = ADungeonGenerator::StaticClass();
	}
	ADungeonGenerator* Dungeon = World->SpawnActor<ADungeonGenerator>(GenClass, FTransform::Identity, SpawnParams);

	// Drop the player into the first room (the generator built itself in its BeginPlay).
	if (Dungeon && Dungeon->GetRoomCount() > 0)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				const FVector Start = Dungeon->GetRoomCenterWorld(0) + FVector(0.f, 0.f, 120.f);
				Pawn->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}
}
