#include "TownGameMode.h"
#include "DayNightCycle.h"

#include "Engine/World.h"
#include "EngineUtils.h"

void ATownGameMode::BuildWorld()
{
	// The town hub — open-sky backdrop (SkyAtmosphere/sun/skylight/fog), boundary walls, distant vista
	// ring, the stations (shop / blackjack / fishing / enter-dungeon portal / bonfire) and scenery — is
	// now authored directly in L_Town as placed, editable actors (built by Tools/build_town.py), not
	// code-spawned. Ensure the base lighting/grade only as a fallback if the map is missing them.
	EnsureLighting();
	EnsurePostProcess();
	EnsureDayNight();
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
