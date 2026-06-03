#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DungeonSaveGame.h"
#include "DungeonGameInstance.generated.h"

class UStatsComponent;
class UInventoryComponent;
class UHotbarComponent;
class USkillTreeComponent;
class UEquipmentComponent;

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

	/** Copy an inventory component's slots into the profile. */
	void CaptureInventory(const UInventoryComponent* Inventory);

	/** Apply the profile's saved inventory onto a component (e.g. on player spawn). */
	void ApplyInventory(UInventoryComponent* Inventory) const;

	/** Copy / apply the action-bar slots. */
	void CaptureHotbar(const UHotbarComponent* Hotbar);
	void ApplyHotbar(UHotbarComponent* Hotbar) const;

	/** Copy / apply the allocated skill-tree nodes. */
	void CaptureSkills(const USkillTreeComponent* Skills);
	void ApplySkills(USkillTreeComponent* Skills) const;

	/** Copy / apply the equipped paperdoll items. */
	void CaptureEquipment(const UEquipmentComponent* Equipment);
	void ApplyEquipment(UEquipmentComponent* Equipment) const;

	/** Record a first-time discovery for the collection log (no duplicates). */
	void AddDiscovered(FName ItemId);

	const TArray<FName>& GetDiscovered() const { return Profile.DiscoveredItems; }
	bool IsDiscovered(FName ItemId) const { return Profile.DiscoveredItems.Contains(ItemId); }

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
