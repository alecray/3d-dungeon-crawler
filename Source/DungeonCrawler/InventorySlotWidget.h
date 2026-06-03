#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UBorder;
class UTextBlock;
class UInventoryComponent;

/**
 * One inventory cell (pure C++): a colored border (rarity) with a stack-count label. Supports
 * drag & drop to move/stack items between slots.
 */
UCLASS()
class DUNGEONCRAWLER_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Configure which inventory + slot this cell represents, then refresh visuals. */
	void Setup(UInventoryComponent* InInventory, int32 InIndex);

	/** Repaint from the current slot contents. */
	void Refresh();

	int32 GetSlotIndex() const { return SlotIndex; }
	UInventoryComponent* GetInventory() const { return Inventory.Get(); }

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Dropped on empty space → drop this slot's item into the world. */
	UFUNCTION()
	void HandleDragCancelled(UDragDropOperation* Operation);

private:
	UPROPERTY() TObjectPtr<UBorder> Box;
	UPROPERTY() TObjectPtr<class UImage> IconImage;
	UPROPERTY() TObjectPtr<UTextBlock> CountText;
	TWeakObjectPtr<UInventoryComponent> Inventory;
	int32 SlotIndex = INDEX_NONE;
};
