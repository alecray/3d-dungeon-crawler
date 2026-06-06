#include "TownGameMode.h"

void ATownGameMode::BuildWorld()
{
	// The town hub — open-sky backdrop (SkyAtmosphere/sun/skylight/fog), boundary walls, distant vista
	// ring, the stations (shop / blackjack / fishing / enter-dungeon portal / bonfire) and scenery — is
	// now authored directly in L_Town as placed, editable actors (built by Tools/build_town.py), not
	// code-spawned. Ensure the base lighting/grade only as a fallback if the map is missing them.
	EnsureLighting();
	EnsurePostProcess();
}
