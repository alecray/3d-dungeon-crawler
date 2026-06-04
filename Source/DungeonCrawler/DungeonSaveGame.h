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

	UPROPERTY() TArray<FName> Hotbar;     // action-bar slot item ids
	UPROPERTY() int32 ActiveSlot = 0;     // selected hotbar slot

	UPROPERTY() TArray<FName> SkillNodes; // allocated skill-tree nodes (one entry per rank)
	UPROPERTY() TArray<FName> Equipment;  // equipped item per paperdoll slot (NAME_None = empty)

	UPROPERTY() TArray<FName> SeenBossIntros; // boss ids whose intro cinematic has already played

	// Settings (pause menu).
	UPROPERTY() float MouseSensitivity = 1.f;
	UPROPERTY() float MasterVolume = 1.f;
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
