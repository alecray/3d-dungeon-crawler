#pragma once

#include "CoreMinimal.h"
#include "DungeonCrawlerGameMode.h"
#include "TownGameMode.generated.h"

/**
 * Game mode for the home-town hub (L_Town). Reuses the base lighting/post-process but does NOT build
 * a dungeon — the town is a saved map (ground, buildings, shop NPC, portal placed in-editor). The
 * player spawns at the level's PlayerStart and carries their profile via the Game Instance.
 */
UCLASS()
class DUNGEONCRAWLER_API ATownGameMode : public ADungeonCrawlerGameMode
{
	GENERATED_BODY()

protected:
	virtual void BuildWorld() override;
};
