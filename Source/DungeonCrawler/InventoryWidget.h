#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UTextBlock;
class UUniformGridPanel;
class UInventoryComponent;
class UInventorySlotWidget;

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

	/** Point this panel at an inventory and label it. Rebuilds the grid. */
	void SetInventory(UInventoryComponent* InInventory, const FString& Title);

	/** Position the panel (canvas offset from screen center). */
	void SetPanelPosition(FVector2D Pos);

private:
	void RebuildGrid();
	void RefreshAll(UInventoryComponent* Changed = nullptr);

	UPROPERTY() TObjectPtr<UBorder> Panel;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY() TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;
	TWeakObjectPtr<UInventoryComponent> Inventory;

	FVector2D PanelPosition = FVector2D::ZeroVector;
	bool bInventorySet = false;

	static constexpr int32 Columns = 6;
};
