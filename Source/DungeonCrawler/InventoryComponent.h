#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

class UInventoryComponent;

/** Fired whenever slots change, so the UI can refresh. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, UInventoryComponent*);

/** Fired the first time an item of rarity >= Rare is acquired (for the collection log). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemDiscovered, FName /*ItemId*/);

/**
 * Fixed-size slot inventory. Adds items (auto-stacking up to MaxStack), supports moving/swapping
 * slots for drag & drop, and reports first-time discoveries of rare+ items.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	/** Adds Count of ItemId, filling existing stacks then empty slots. Returns the amount that
	 *  didn't fit. */
	int32 AddItem(FName ItemId, int32 Count = 1);

	/** Moves/swaps/merges the stack at From into To (for drag & drop). */
	void MoveItem(int32 From, int32 To);

	/** Removes up to Count from a slot. */
	void RemoveAt(int32 Index, int32 Count = 1);

	int32 NumSlots() const { return Slots.Num(); }
	const FInventorySlot& GetSlot(int32 Index) const { return Slots[Index]; }
	const TArray<FInventorySlot>& GetSlots() const { return Slots; }

	void SetSlots(const TArray<FInventorySlot>& In);

	FOnInventoryChanged OnInventoryChanged;
	FOnItemDiscovered OnItemDiscovered;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	int32 SlotCount = 24;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<FInventorySlot> Slots;

private:
	void NotifyChanged() { OnInventoryChanged.Broadcast(this); }
};
