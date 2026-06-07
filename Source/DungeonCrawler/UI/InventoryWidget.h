#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UTextBlock;
class UUniformGridPanel;
class UHorizontalBox;
class UInventoryComponent;
class UInventorySlotWidget;
class UEquipmentSlotWidget;

/**
 * A draggable slot-grid panel (pure C++). Used both for the player's inventory and for a chest's
 * contents — loot by dragging items from one grid into the other. Bind via SetInventory().
 */
UCLASS()
class DUNGEONCRAWLER_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Absorbs drops that miss a slot but land inside the window, so the item returns to its slot instead
	 *  of being dropped to the world. Only drops OUTSIDE the window reach OnDragCancelled (world drop). */
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Point this panel at an inventory and label it. Rebuilds the grid. */
	void SetInventory(UInventoryComponent* InInventory, const FString& Title);

	/** Position the panel (canvas offset from screen center). */
	void SetPanelPosition(FVector2D Pos);

	/** Show the paperdoll equipment panel beside the grid (the player's own inventory screen). */
	void SetShowEquipment(bool bShow);

private:
	void RebuildGrid();
	void RefreshAll(UInventoryComponent* Changed = nullptr);
	void BuildEquipmentPanel();
	void RefreshEquipment();
	/** Updates the gold readout (shown only on the player's own inventory screen). */
	void RefreshGold();

	UPROPERTY() TObjectPtr<UBorder> Panel;
	UPROPERTY() TObjectPtr<UHorizontalBox> Row;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> GoldText;
	UPROPERTY() TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY() TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;
	UPROPERTY() TObjectPtr<UCanvasPanel> EquipCanvas;
	UPROPERTY() TObjectPtr<class UBorder> EquipSection;
	UPROPERTY() TArray<TObjectPtr<UEquipmentSlotWidget>> EquipSlotWidgets;
	TWeakObjectPtr<UInventoryComponent> Inventory;

	FVector2D PanelPosition = FVector2D::ZeroVector;
	bool bInventorySet = false;
	bool bShowEquipment = false;

	static constexpr int32 Columns = 6;
};
