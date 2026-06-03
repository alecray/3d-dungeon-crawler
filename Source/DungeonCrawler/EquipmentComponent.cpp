#include "EquipmentComponent.h"
#include "StatsComponent.h"
#include "InventoryComponent.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Equipped.Num() != ESlot::NumSlots)
	{
		Equipped.Init(NAME_None, ESlot::NumSlots);
	}
}

EEquipSlot UEquipmentComponent::SlotCategory(int32 SlotIndex)
{
	switch (SlotIndex)
	{
	case ESlot::Head:   return EEquipSlot::Head;
	case ESlot::Amulet: return EEquipSlot::Amulet;
	case ESlot::Body:   return EEquipSlot::Body;
	case ESlot::Gloves: return EEquipSlot::Gloves;
	case ESlot::Belt:   return EEquipSlot::Belt;
	case ESlot::Legs:   return EEquipSlot::Legs;
	case ESlot::Feet:   return EEquipSlot::Feet;
	case ESlot::Ring1:
	case ESlot::Ring2:
	case ESlot::Ring3:
	case ESlot::Ring4:  return EEquipSlot::Ring;
	default:            return EEquipSlot::None;
	}
}

FName UEquipmentComponent::GetEquipped(int32 SlotIndex) const
{
	return Equipped.IsValidIndex(SlotIndex) ? Equipped[SlotIndex] : NAME_None;
}

bool UEquipmentComponent::CanEquip(int32 SlotIndex, FName ItemId) const
{
	if (!Equipped.IsValidIndex(SlotIndex) || !ItemDatabase::Contains(ItemId))
	{
		return false;
	}
	return ItemDatabase::Get(ItemId).EquipSlot == SlotCategory(SlotIndex);
}

bool UEquipmentComponent::EquipFromInventory(int32 SlotIndex, UInventoryComponent* Inventory, int32 InvSlotIndex)
{
	if (!Inventory || !Equipped.IsValidIndex(SlotIndex) || !Inventory->GetSlots().IsValidIndex(InvSlotIndex))
	{
		return false;
	}
	const FInventorySlot& Src = Inventory->GetSlot(InvSlotIndex);
	if (Src.IsEmpty() || !CanEquip(SlotIndex, Src.ItemId))
	{
		return false;
	}

	const FName NewItem = Src.ItemId;
	const FName Previous = Equipped[SlotIndex];

	Inventory->RemoveAt(InvSlotIndex, 1); // take one from the stack
	Equipped[SlotIndex] = NewItem;
	if (!Previous.IsNone())
	{
		Inventory->AddItem(Previous, 1); // swap the old item back into the bag
	}

	RecomputeBonuses();
	OnEquipmentChanged.Broadcast();
	return true;
}

bool UEquipmentComponent::UnequipToInventory(int32 SlotIndex, UInventoryComponent* Inventory)
{
	if (!Inventory || !Equipped.IsValidIndex(SlotIndex) || Equipped[SlotIndex].IsNone())
	{
		return false;
	}
	const FName Item = Equipped[SlotIndex];
	const int32 Leftover = Inventory->AddItem(Item, 1);
	if (Leftover > 0)
	{
		return false; // inventory full: keep it equipped
	}
	Equipped[SlotIndex] = NAME_None;
	RecomputeBonuses();
	OnEquipmentChanged.Broadcast();
	return true;
}

void UEquipmentComponent::LoadFrom(const TArray<FName>& InSlots)
{
	Equipped.Init(NAME_None, ESlot::NumSlots);
	for (int32 i = 0; i < ESlot::NumSlots && i < InSlots.Num(); ++i)
	{
		Equipped[i] = InSlots[i];
	}
	RecomputeBonuses();
	OnEquipmentChanged.Broadcast();
}

void UEquipmentComponent::RecomputeBonuses()
{
	UStatsComponent* Stats = GetStats();
	if (!Stats)
	{
		return;
	}

	FItemBonuses Total;
	for (const FName& Id : Equipped)
	{
		if (Id.IsNone() || !ItemDatabase::Contains(Id))
		{
			continue;
		}
		const FItemBonuses& B = ItemDatabase::Get(Id).Bonuses;
		Total.MaxHealth  += B.MaxHealth;
		Total.MaxMana    += B.MaxMana;
		Total.MaxStamina += B.MaxStamina;
		Total.MeleeMult  += B.MeleeMult;
		Total.RangedMult += B.RangedMult;
		Total.SpellMult  += B.SpellMult;
	}

	Stats->EquipBonusMaxHealth  = Total.MaxHealth;
	Stats->EquipBonusMaxMana    = Total.MaxMana;
	Stats->EquipBonusMaxStamina = Total.MaxStamina;
	Stats->EquipBonusMeleeMult  = Total.MeleeMult;
	Stats->EquipBonusRangedMult = Total.RangedMult;
	Stats->EquipBonusSpellMult  = Total.SpellMult;
	Stats->NotifyChanged();
}

UStatsComponent* UEquipmentComponent::GetStats() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UStatsComponent>() : nullptr;
}
