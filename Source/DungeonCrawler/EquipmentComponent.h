#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "EquipmentComponent.generated.h"

class UStatsComponent;
class UInventoryComponent;

/** Broadcast whenever equipped items change, so the equipment UI can refresh. */
DECLARE_MULTICAST_DELEGATE(FOnEquipmentChanged);

/**
 * Paperdoll equipment: one item per slot (head/amulet/body/gloves/belt/legs/feet + 4 rings). Items
 * may only go in a slot matching their EEquipSlot (rings fit any ring slot). Equipping/unequipping
 * moves items to/from the inventory and re-applies the summed stat bonuses to UStatsComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	/** Fixed slot order (indices into the equipped array). */
	enum ESlot : int32 { Head, Amulet, Body, Gloves, Belt, Legs, Feet, Ring1, Ring2, Ring3, Ring4, NumSlots };

	/** The EEquipSlot category a given slot index accepts. */
	static EEquipSlot SlotCategory(int32 SlotIndex);

	int32 NumEquipSlots() const { return ESlot::NumSlots; }
	FName GetEquipped(int32 SlotIndex) const;

	/** True if the item may be placed in this slot (its EquipSlot matches the slot's category). */
	bool CanEquip(int32 SlotIndex, FName ItemId) const;

	/** Moves the item at InvSlotIndex into the equipment slot (swapping any current item back to the
	 *  inventory). Returns true on success. */
	bool EquipFromInventory(int32 SlotIndex, UInventoryComponent* Inventory, int32 InvSlotIndex);

	/** Returns the equipped item to the inventory and clears the slot. Returns true on success. */
	bool UnequipToInventory(int32 SlotIndex, UInventoryComponent* Inventory);

	/** Save/load: the equipped item id per slot (NAME_None = empty). */
	TArray<FName> GetSlots() const { return Equipped; }
	void LoadFrom(const TArray<FName>& InSlots);

	FOnEquipmentChanged OnEquipmentChanged;

protected:
	virtual void BeginPlay() override;

private:
	void RecomputeBonuses();
	UStatsComponent* GetStats() const;

	UPROPERTY() TArray<FName> Equipped; // size == NumSlots
};
