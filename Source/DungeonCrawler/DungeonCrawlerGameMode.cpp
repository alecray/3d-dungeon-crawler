#include "DungeonCrawlerGameMode.h"
#include "DungeonGenerator.h"
#include "FirstPersonCharacter.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

ADungeonCrawlerGameMode::ADungeonCrawlerGameMode()
{
	DefaultPawnClass = AFirstPersonCharacter::StaticClass();
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
				SC->SetIntensity(1.5f);
				SC->SetLightColor(FLinearColor(0.55f, 0.6f, 0.75f)); // soft blue ambient
				SC->bLowerHemisphereIsBlack = false;                 // fill the floor/shadows too
				SC->LowerHemisphereColor = FLinearColor(0.25f, 0.25f, 0.3f);
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

	// Lock exposure so the view doesn't harshly auto-adjust between bright and dark spots.
	S.bOverride_AutoExposureMinBrightness = true;
	S.AutoExposureMinBrightness = 1.0f;
	S.bOverride_AutoExposureMaxBrightness = true;
	S.AutoExposureMaxBrightness = 1.0f;

	// Ease ambient occlusion so contact shadows aren't a hard black line.
	S.bOverride_AmbientOcclusionIntensity = true;
	S.AmbientOcclusionIntensity = 0.4f;
	S.bOverride_AmbientOcclusionRadius = true;
	S.AmbientOcclusionRadius = 80.f;

	// Lift indirect bounce a touch and soften the highlights for a gentle, stylized look.
	S.bOverride_IndirectLightingIntensity = true;
	S.IndirectLightingIntensity = 1.4f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.4f;
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
