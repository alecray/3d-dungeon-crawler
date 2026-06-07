#include "InventoryComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Slots.Num() != SlotCount)
	{
		Slots.SetNum(SlotCount);
	}
}

int32 UInventoryComponent::AddItem(FName ItemId, int32 Count)
{
	if (!ItemDatabase::Contains(ItemId) || Count <= 0)
	{
		return Count;
	}

	const FItemDef& Def = ItemDatabase::Get(ItemId);
	const int32 MaxStack = FMath::Max(1, Def.MaxStack);
	int32 Remaining = Count;

	// First top up existing stacks of the same item.
	for (FInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0) { break; }
		if (Slot.ItemId == ItemId && Slot.Count < MaxStack)
		{
			const int32 Room = MaxStack - Slot.Count;
			const int32 Added = FMath::Min(Room, Remaining);
			Slot.Count += Added;
			Remaining -= Added;
		}
	}

	// Then fill empty slots.
	for (FInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0) { break; }
		if (Slot.IsEmpty())
		{
			Slot.ItemId = ItemId;
			const int32 Added = FMath::Min(MaxStack, Remaining);
			Slot.Count = Added;
			Remaining -= Added;
		}
	}

	if (Remaining < Count)
	{
		// Something was added: notify, and flag rare+ discoveries for the collection log.
		if (Def.Rarity >= EItemRarity::Rare)
		{
			OnItemDiscovered.Broadcast(ItemId);
		}
		NotifyChanged();
	}
	return Remaining;
}

void UInventoryComponent::MoveItem(int32 From, int32 To)
{
	if (!Slots.IsValidIndex(From) || !Slots.IsValidIndex(To) || From == To)
	{
		return;
	}

	FInventorySlot& A = Slots[From];
	FInventorySlot& B = Slots[To];
	if (A.IsEmpty())
	{
		return;
	}

	// Merge onto a matching stack when possible, otherwise swap.
	if (!B.IsEmpty() && B.ItemId == A.ItemId)
	{
		const int32 MaxStack = FMath::Max(1, ItemDatabase::Get(A.ItemId).MaxStack);
		const int32 Room = MaxStack - B.Count;
		const int32 Moved = FMath::Min(Room, A.Count);
		B.Count += Moved;
		A.Count -= Moved;
		if (A.Count <= 0) { A.Clear(); }
	}
	else
	{
		Swap(A, B);
	}
	NotifyChanged();
}

void UInventoryComponent::RemoveAt(int32 Index, int32 Count)
{
	if (!Slots.IsValidIndex(Index) || Slots[Index].IsEmpty() || Count <= 0)
	{
		return;
	}
	Slots[Index].Count -= Count;
	if (Slots[Index].Count <= 0)
	{
		Slots[Index].Clear();
	}
	NotifyChanged();
}

int32 UInventoryComponent::GetItemCount(FName ItemId) const
{
	int32 Total = 0;
	for (const FInventorySlot& S : Slots)
	{
		if (S.ItemId == ItemId)
		{
			Total += S.Count;
		}
	}
	return Total;
}

void UInventoryComponent::SetSlots(const TArray<FInventorySlot>& In)
{
	Slots = In;
	Slots.SetNum(SlotCount);
	NotifyChanged();
}
