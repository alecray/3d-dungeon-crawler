#include "HotbarComponent.h"

UHotbarComponent::UHotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHotbarComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureSize();
}

void UHotbarComponent::EnsureSize()
{
	if (Slots.Num() != NumSlots)
	{
		Slots.SetNum(NumSlots);
	}
	ActiveIndex = FMath::Clamp(ActiveIndex, 0, NumSlots - 1);
}

void UHotbarComponent::SetSlot(int32 Index, FName ItemId)
{
	EnsureSize();
	if (Slots.IsValidIndex(Index))
	{
		Slots[Index] = ItemId;
		OnHotbarChanged.Broadcast(this);
	}
}

void UHotbarComponent::SelectSlot(int32 Index)
{
	EnsureSize();
	if (Slots.IsValidIndex(Index) && Index != ActiveIndex)
	{
		ActiveIndex = Index;
		OnHotbarChanged.Broadcast(this);
	}
	else if (Slots.IsValidIndex(Index))
	{
		// Re-selecting the active slot still re-broadcasts (lets the player re-equip if needed).
		OnHotbarChanged.Broadcast(this);
	}
}

void UHotbarComponent::LoadFrom(const TArray<FName>& InSlots, int32 InActive)
{
	Slots = InSlots;
	EnsureSize();
	ActiveIndex = FMath::Clamp(InActive, 0, NumSlots - 1);
	OnHotbarChanged.Broadcast(this);
}
