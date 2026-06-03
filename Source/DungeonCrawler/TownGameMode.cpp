#include "TownGameMode.h"

void ATownGameMode::BuildWorld()
{
	// Town is a saved level; just make sure it has the stylized lighting/grade. The player spawns at
	// the map's PlayerStart (no dungeon, no pawn relocation).
	EnsureLighting();
	EnsurePostProcess();
}
