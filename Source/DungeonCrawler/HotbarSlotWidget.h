#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarSlotWidget.generated.h"

class UBorder;
class UTextBlock;
class UHotbarComponent;

/**
 * One action-bar cell: shows its key number and the assigned item's rarity color, highlighted when
 * active. Click to select (equip); drop an inventory item on it to assign.
 */
UCLASS()
class DUNGEONCRAWLER_API UHotbarSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	void Setup(UHotbarComponent* InHotbar, int32 InIndex);
	void Refresh();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY() TObjectPtr<UBorder> Highlight;
	UPROPERTY() TObjectPtr<UBorder> Inner;
	UPROPERTY() TObjectPtr<UTextBlock> KeyLabel;
	TWeakObjectPtr<UHotbarComponent> Hotbar;
	int32 Index = INDEX_NONE;
};
