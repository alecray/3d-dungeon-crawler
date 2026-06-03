#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "StatsComponent.h" // FCoreAttributes
#include "ItemTypes.h"      // FInventorySlot
#include "DungeonSaveGame.generated.h"

/**
 * The persistent player profile carried between levels (town ⇄ dungeon) and written to disk.
 * Later phases extend this with inventory, collection log, and skill-tree allocations.
 */
USTRUCT(BlueprintType)
struct FPlayerProfile
{
	GENERATED_BODY()

	UPROPERTY() bool bInitialized = false;

	UPROPERTY() FCoreAttributes Attributes;
	UPROPERTY() int32 Level = 1;
	UPROPERTY() int32 XP = 0;
	UPROPERTY() int32 SkillPoints = 0;
	UPROPERTY() int32 AttributePoints = 0;
	UPROPERTY() int32 Gold = 0;

	UPROPERTY() TArray<FInventorySlot> Inventory;
	UPROPERTY() TArray<FName> DiscoveredItems; // collection log
};

/** SaveGame wrapper around the profile, serialized via UGameplayStatics::SaveGameToSlot. */
UCLASS()
class DUNGEONCRAWLER_API UDungeonSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPlayerProfile Profile;
};
