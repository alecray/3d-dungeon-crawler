#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DungeonSaveGame.h"
#include "DungeonGameInstance.generated.h"

class UStatsComponent;

/**
 * Holds the player profile in memory across level transitions and brokers save/load to disk.
 * The player pulls its stats from here on spawn and pushes them back before saving/traveling.
 */
UCLASS()
class DUNGEONCRAWLER_API UDungeonGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** The live profile other systems read/write during play. */
	FPlayerProfile& GetProfile() { return Profile; }

	/** Copy a stats component's values into the in-memory profile. */
	void CaptureFromStats(const UStatsComponent* Stats, int32 Gold);

	/** Apply the in-memory profile onto a stats component (e.g. on player spawn). */
	void ApplyToStats(UStatsComponent* Stats) const;

	/** Write the in-memory profile to the save slot. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveProfile();

	/** Read the save slot into the in-memory profile (called on Init). */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadProfile();

	bool HasProfile() const { return Profile.bInitialized; }

protected:
	UPROPERTY()
	FPlayerProfile Profile;

	static const TCHAR* SlotName() { return TEXT("DungeonProfile"); }
};
