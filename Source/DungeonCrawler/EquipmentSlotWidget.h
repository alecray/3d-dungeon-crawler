#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EquipmentSlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UEquipmentComponent;

/**
 * One paperdoll equipment slot. Shows its slot label when empty (e.g. "head") and the equipped item's
 * icon when filled. Drop an inventory item here to equip it (only if its type fits the slot); click to
 * unequip back to the inventory.
 */
UCLASS()
class DUNGEONCRAWLER_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	void Setup(UEquipmentComponent* InEquipment, int32 InSlotIndex);
	void Refresh();

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UEquipmentComponent* GetEquipment() const;

	UPROPERTY() TObjectPtr<UBorder> Box;
	UPROPERTY() TObjectPtr<UImage> IconImage;
	UPROPERTY() TObjectPtr<UTextBlock> LabelText;

	UPROPERTY() TWeakObjectPtr<UEquipmentComponent> Equipment;
	int32 SlotIndex = 0;
};
