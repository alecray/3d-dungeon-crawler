#include "HotbarSlotWidget.h"
#include "HotbarComponent.h"
#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"

bool UHotbarSlotWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<USizeBox>(WidgetTree->RootWidget))
	{
		return true;
	}

	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
	Root->SetWidthOverride(60.f);
	Root->SetHeightOverride(60.f);
	WidgetTree->RootWidget = Root;

	Highlight = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Highlight"));
	Highlight->SetPadding(FMargin(3.f));
	Root->AddChild(Highlight);

	Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Inner"));
	Highlight->AddChild(Inner);

	KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Key"));
	Inner->AddChild(KeyLabel);

	return true;
}

void UHotbarSlotWidget::Setup(UHotbarComponent* InHotbar, int32 InIndex)
{
	Hotbar = InHotbar;
	Index = InIndex;
	if (KeyLabel)
	{
		KeyLabel->SetText(FText::AsNumber(Index + 1));
	}
	Refresh();
}

void UHotbarSlotWidget::Refresh()
{
	if (!Highlight || !Inner || !Hotbar.IsValid())
	{
		return;
	}

	const bool bActive = (Hotbar->GetActiveIndex() == Index);
	Highlight->SetBrushColor(bActive ? FLinearColor(1.f, 0.85f, 0.2f) : FLinearColor(0.f, 0.f, 0.f, 0.6f));

	const FName ItemId = Hotbar->GetSlotItem(Index);
	if (ItemDatabase::Contains(ItemId))
	{
		Inner->SetBrushColor(RarityColor(ItemDatabase::Get(ItemId).Rarity));
	}
	else
	{
		Inner->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.11f, 0.9f));
	}
}

FReply UHotbarSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Hotbar.IsValid())
	{
		Hotbar->SelectSlot(Index); // equip what's in this slot
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool UHotbarSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation || !Hotbar.IsValid())
	{
		return false;
	}
	// Assign the dragged inventory item to this hotbar slot (a reference; doesn't consume the item).
	if (UInventorySlotWidget* Source = Cast<UInventorySlotWidget>(InOperation->Payload))
	{
		if (UInventoryComponent* SrcInv = Source->GetInventory())
		{
			const FName ItemId = SrcInv->GetSlot(Source->GetSlotIndex()).ItemId;
			if (!ItemId.IsNone())
			{
				Hotbar->SetSlot(Index, ItemId);
				return true;
			}
		}
	}
	return false;
}
