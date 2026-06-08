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

	/** Lifetime play statistics (Stats screen). Increment fields directly; call SaveProfile() to persist. */
	FPlayerStats& GetStats() { return Profile.Stats; }
	const FPlayerStats& GetStats() const { return Profile.Stats; }

	/** Copy a stats component's values into the in-memory profile. (Gold is owned here; see GrantGold/SpendGold.) */
	void CaptureFromStats(const UStatsComponent* Stats);

	// ---- Gold (lives in the saved profile; the player pawn forwards GetGold/AddGold/TrySpendGold to these) ----
	int32 GetGold() const { return Profile.Gold; }
	/** Add gold (clamped >= 0); positive amounts also count toward the GoldLooted stat. Does NOT save on its
	    own — the caller persists the profile (so a gold change saves alongside the rest of the live state). */
	void GrantGold(int32 Amount);
	/** Spend gold if affordable (returns false with no change otherwise). Does NOT save on its own. */
	bool SpendGold(int32 Amount);

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

	/** True once a boss type's intro cinematic has played (persisted, so it only plays the first time). */
	bool HasSeenBossIntro(FName BossId) const { return Profile.SeenBossIntros.Contains(BossId); }

	/** Records that a boss type's intro cinematic has played, and writes the profile to disk. */
	void MarkBossIntroSeen(FName BossId);

	/** Write the in-memory profile to the save slot. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveProfile();

	/** Read the save slot into the in-memory profile (called on Init). */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadProfile();

	bool HasProfile() const { return Profile.bInitialized; }

	/** Version string ("v0.1.N") read from DefaultGame.ini ProjectVersion, for on-screen display. */
	static FString GetGameVersion();

	/** Per-session (not saved): true once the start menu has been dismissed, so it only shows on boot. */
	bool IsSessionStarted() const { return bSessionStarted; }
	void SetSessionStarted() { bSessionStarted = true; }

	// ---- Transient dungeon-entry selection (NOT saved to disk; cleared each session) ----
	/** The difficulty tier the player selected in the map/tier menu (0 = default/easy, 1-3 = scaling difficulty). */
	int32 GetSelectedTier() const { return SelectedTier; }
	/** Clamp to 0..3. */
	void SetSelectedTier(int32 Tier) { SelectedTier = FMath::Clamp(Tier, 0, 3); }

	/** The map FName the player selected in the map/tier menu (e.g. "L_DungeonTest"). */
	FName GetSelectedMap() const { return SelectedMap; }
	void SetSelectedMap(FName Map) { SelectedMap = Map; }

protected:
	UPROPERTY()
	FPlayerProfile Profile;

	bool bSessionStarted = false;

	/** Transient: cleared each boot, written by the MapSelectWidget before travel. */
	int32 SelectedTier = 0;
	FName SelectedMap;  // NAME_None until the player picks one

	static const TCHAR* SlotName() { return TEXT("DungeonProfile"); }
};
