#include "HotbarSlotWidget.h"
#include "HotbarComponent.h"
#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "ItemIconSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
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
	Root->SetWidthOverride(84.f);
	Root->SetHeightOverride(84.f);
	WidgetTree->RootWidget = Root;

	Highlight = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Highlight"));
	Highlight->SetPadding(FMargin(3.f));
	Root->AddChild(Highlight);

	Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Inner"));
	Highlight->AddChild(Inner);

	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
	Inner->AddChild(Stack);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
	IconImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* IconSlot = Stack->AddChildToOverlay(IconImage))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
	}

	KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Key"));
	if (UOverlaySlot* KeySlot = Stack->AddChildToOverlay(KeyLabel))
	{
		KeySlot->SetHorizontalAlignment(HAlign_Left); // key number top-left
		KeySlot->SetVerticalAlignment(VAlign_Top);
	}

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
	const bool bHasItem = ItemDatabase::Contains(ItemId);
	Inner->SetBrushColor(bHasItem ? RarityColor(ItemDatabase::Get(ItemId).Rarity)
		: FLinearColor(0.1f, 0.1f, 0.11f, 0.9f));

	UTexture2D* Icon = nullptr;
	if (bHasItem)
	{
		if (UWorld* World = GetWorld())
		{
			if (UItemIconSubsystem* Icons = World->GetSubsystem<UItemIconSubsystem>())
			{
				Icon = Icons->GetIcon(ItemId);
			}
		}
	}
	if (IconImage)
	{
		if (Icon)
		{
			IconImage->SetBrushResourceObject(Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
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
