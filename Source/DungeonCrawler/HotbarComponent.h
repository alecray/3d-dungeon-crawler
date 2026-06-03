#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HotbarComponent.generated.h"

class UHotbarComponent;

/** Fired when slot contents or the active selection change (UI refresh / re-equip). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHotbarChanged, UHotbarComponent*);

/**
 * 8-slot action bar. Each slot references an item id (e.g. a weapon); the active slot determines the
 * equipped weapon. Items are assigned by dragging from the inventory; slots are selected by click or
 * number keys 1–8.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UHotbarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHotbarComponent();

	static constexpr int32 NumSlots = 8;

	void SetSlot(int32 Index, FName ItemId);
	void SelectSlot(int32 Index);

	FName GetSlotItem(int32 Index) const { return Slots.IsValidIndex(Index) ? Slots[Index] : NAME_None; }
	int32 GetActiveIndex() const { return ActiveIndex; }
	FName GetActiveItem() const { return GetSlotItem(ActiveIndex); }

	const TArray<FName>& GetSlots() const { return Slots; }
	void LoadFrom(const TArray<FName>& InSlots, int32 InActive);

	FOnHotbarChanged OnHotbarChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Hotbar")
	TArray<FName> Slots;

	UPROPERTY(VisibleAnywhere, Category = "Hotbar")
	int32 ActiveIndex = 0;

private:
	void EnsureSize();
};
