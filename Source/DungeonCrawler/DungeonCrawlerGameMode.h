#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DungeonCrawlerGameMode.generated.h"

class ADungeonGenerator;

/**
 * Builds the whole prototype world at runtime: spawns the procedural dungeon generator and basic
 * lighting in BeginPlay, then drops the player into the first room. No level assets required.
 */
UCLASS()
class DUNGEONCRAWLER_API ADungeonCrawlerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADungeonCrawlerGameMode();

	virtual void BeginPlay() override;

protected:
	/** Generator class to spawn (defaults to ADungeonGenerator). */
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon")
	TSubclassOf<ADungeonGenerator> DungeonGeneratorClass;

private:
	void EnsureLighting();
	void BuildWorld();
};
