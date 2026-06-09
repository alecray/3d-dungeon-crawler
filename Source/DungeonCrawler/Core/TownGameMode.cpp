#include "TownGameMode.h"
#include "DayNightCycle.h"
#include "MonsterSpawnPedestal.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

void ATownGameMode::BuildWorld()
{
	// The town hub — open-sky backdrop (SkyAtmosphere/sun/skylight/fog), boundary walls, distant vista
	// ring, the stations (shop / blackjack / fishing / enter-dungeon portal / bonfire) and scenery — is
	// now authored directly in L_Town as placed, editable actors (built by Tools/build_town.py), not
	// code-spawned. Ensure the base lighting/grade only as a fallback if the map is missing them.
	EnsureLighting();
	EnsurePostProcess();
	EnsureDayNight();
	EnsureSpawnPedestal();
}

void ATownGameMode::EnsureDayNight()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ADayNightCycle> It(World); It; ++It)
	{
		return; // already present
	}
	// Spawned (not saved into L_Town) — it finds the placed sun/sky/fog and drives the cycle. The town's
	// hand-edited scene is left untouched.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ADayNightCycle>(ADayNightCycle::StaticClass(), FTransform::Identity, Params);
}

void ATownGameMode::EnsureSpawnPedestal()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AMonsterSpawnPedestal> It(World); It; ++It)
	{
		return; // a pedestal was placed in the level — leave it.
	}

	// Drop one a few metres in front of the PlayerStart so it's right there when you load the town.
	FVector Loc(0.f, 0.f, 100.f);
	FRotator Rot = FRotator::ZeroRotator;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const FTransform T = It->GetActorTransform();
		Loc = T.GetLocation() + T.GetRotation().GetForwardVector() * 400.f + T.GetRotation().GetRightVector() * 300.f;
		Rot = FRotator(0.f, T.Rotator().Yaw + 180.f, 0.f); // face back toward the player's spawn
		break;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World->SpawnActor<AMonsterSpawnPedestal>(AMonsterSpawnPedestal::StaticClass(), FTransform(Rot, Loc), Params);
}
