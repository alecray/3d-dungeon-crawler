#include "DungeonCrawlerGameMode.h"
#include "DungeonGenerator.h"
#include "FirstPersonCharacter.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
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
		// Dim fill so the interior isn't pitch black between the per-room point lights.
		if (ADirectionalLight* Key = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(), FTransform(FRotator(-50.f, -40.f, 0.f), FVector::ZeroVector), Params))
		{
			if (ULightComponent* L = Key->GetLightComponent())
			{
				L->SetMobility(EComponentMobility::Movable);
				L->SetIntensity(0.4f);
			}
		}
	}

	if (!bHasSky)
	{
		if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(), FTransform(FVector::ZeroVector), Params))
		{
			if (USkyLightComponent* SC = Sky->GetLightComponent())
			{
				SC->SetMobility(EComponentMobility::Movable);
				SC->SetIntensity(0.6f);
				SC->RecaptureSky();
			}
		}
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
