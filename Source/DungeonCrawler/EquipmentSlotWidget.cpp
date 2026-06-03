#include "EquipmentSlotWidget.h"
#include "EquipmentComponent.h"
#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"
#include "ItemIconSubsystem.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Input/Reply.h"

bool UEquipmentSlotWidget::Initialize()
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

	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
	Box->AddChild(Stack);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
	IconImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* IconSlot = Stack->AddChildToOverlay(IconImage))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
	}

	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
	LabelText->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* LabelSlot = Stack->AddChildToOverlay(LabelText))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	return true;
}

void UEquipmentSlotWidget::Setup(UEquipmentComponent* InEquipment, int32 InSlotIndex)
{
	Equipment = InEquipment;
	SlotIndex = InSlotIndex;
	Refresh();
}

void UEquipmentSlotWidget::Refresh()
{
	if (!Box || !Equipment.IsValid())
	{
		return;
	}

	const FName ItemId = Equipment->GetEquipped(SlotIndex);
	if (ItemId.IsNone() || !ItemDatabase::Contains(ItemId))
	{
		// Empty: show the slot's category label (e.g. "head") on a dim cell.
		Box->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.09f, 0.9f));
		if (IconImage) { IconImage->SetVisibility(ESlateVisibility::Collapsed); }
		if (LabelText)
		{
			LabelText->SetText(FText::FromString(EquipSlotLabel(UEquipmentComponent::SlotCategory(SlotIndex))));
			LabelText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		SetToolTipText(FText::GetEmpty());
		return;
	}

	const FItemDef& Def = ItemDatabase::Get(ItemId);
	Box->SetBrushColor(RarityColor(Def.Rarity));
	if (LabelText) { LabelText->SetVisibility(ESlateVisibility::Collapsed); }

	UTextureRenderTarget2D* Icon = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UItemIconSubsystem* Icons = World->GetSubsystem<UItemIconSubsystem>())
		{
			Icon = Icons->GetIcon(ItemId);
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

	FString Tip = FString::Printf(TEXT("%s\n%s %s"), *Def.DisplayName, *RarityName(Def.Rarity), *ItemTypeName(Def.Type));
	if (!Def.Description.IsEmpty()) { Tip += FString::Printf(TEXT("\n\n%s"), *Def.Description); }
	Tip += TEXT("\n\n(click to unequip)");
	SetToolTipText(FText::FromString(Tip));
}

FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Click an equipped slot to unequip it back to the inventory.
	if (Equipment.IsValid() && !Equipment->GetEquipped(SlotIndex).IsNone())
	{
		if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
		{
			Equipment->UnequipToInventory(SlotIndex, Player->GetInventoryComponent());
			Refresh();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation || !Equipment.IsValid())
	{
		return false;
	}
	UInventorySlotWidget* Source = Cast<UInventorySlotWidget>(InOperation->Payload);
	if (!Source || !Source->GetInventory())
	{
		return false;
	}
	// Equip the dragged inventory item if it fits this slot (CanEquip checks the type).
	const bool bEquipped = Equipment->EquipFromInventory(SlotIndex, Source->GetInventory(), Source->GetSlotIndex());
	Refresh();
	return bEquipped;
}

UEquipmentComponent* UEquipmentSlotWidget::GetEquipment() const
{
	return Equipment.Get();
}
