#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"

bool UInventorySlotWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<USizeBox>(WidgetTree->RootWidget))
	{
		return true;
	}

	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSize"));
	Root->SetWidthOverride(58.f);
	Root->SetHeightOverride(58.f);
	WidgetTree->RootWidget = Root;

	Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBox"));
	Box->SetPadding(FMargin(2.f));
	Root->AddChild(Box);

	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Count"));
	Box->AddChild(CountText);

	return true;
}

void UInventorySlotWidget::Setup(UInventoryComponent* InInventory, int32 InIndex)
{
	Inventory = InInventory;
	SlotIndex = InIndex;
	Refresh();
}

void UInventorySlotWidget::Refresh()
{
	if (!Box || !CountText || !Inventory.IsValid() || !Inventory->GetSlots().IsValidIndex(SlotIndex))
	{
		return;
	}

	const FInventorySlot& InvSlot = Inventory->GetSlot(SlotIndex);
	if (InvSlot.IsEmpty())
	{
		Box->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.09f, 0.85f)); // empty cell
		CountText->SetText(FText::GetEmpty());
	}
	else
	{
		const FItemDef& Def = ItemDatabase::Get(InvSlot.ItemId);
		Box->SetBrushColor(RarityColor(Def.Rarity));
		CountText->SetText(InvSlot.Count > 1 ? FText::AsNumber(InvSlot.Count) : FText::GetEmpty());
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Begin a drag on left-press (only meaningful if this slot holds something).
	if (Inventory.IsValid() && Inventory->GetSlots().IsValidIndex(SlotIndex) && !Inventory->GetSlot(SlotIndex).IsEmpty())
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDragDropOperation* Op = NewObject<UDragDropOperation>(GetTransientPackage());
	Op->Payload = this; // carries the source slot
	Op->Pivot = EDragPivot::MouseDown;

	// Simple floating visual: a colored border tinted to the dragged item's rarity.
	if (Inventory.IsValid() && WidgetTree)
	{
		const FInventorySlot& InvSlot = Inventory->GetSlot(SlotIndex);
		UBorder* Vis = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Vis->SetBrushColor(RarityColor(ItemDatabase::Get(InvSlot.ItemId).Rarity));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(ItemDatabase::Get(InvSlot.ItemId).DisplayName));
		Vis->AddChild(Label);
		Op->DefaultDragVisual = Vis;
	}

	OutOperation = Op;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation)
	{
		return false;
	}
	UInventorySlotWidget* Source = Cast<UInventorySlotWidget>(InOperation->Payload);
	if (Source && Inventory.IsValid())
	{
		Inventory->MoveItem(Source->GetSlotIndex(), SlotIndex);
		return true;
	}
	return false;
}
