#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"
#include "ItemPickup.h"

#include "ItemIconSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
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

	// Overlay so the rendered icon and the stack count can stack inside the border.
	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
	Box->AddChild(Stack);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
	IconImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* IconSlot = Stack->AddChildToOverlay(IconImage))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Fill); // icon fills the slot
		IconSlot->SetVerticalAlignment(VAlign_Fill);
	}

	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Count"));
	if (UOverlaySlot* CountSlot = Stack->AddChildToOverlay(CountText))
	{
		CountSlot->SetHorizontalAlignment(HAlign_Right); // count sits bottom-right
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
	}

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
		if (IconImage) { IconImage->SetVisibility(ESlateVisibility::Collapsed); }
	}
	else
	{
		const FItemDef& Def = ItemDatabase::Get(InvSlot.ItemId);
		Box->SetBrushColor(RarityColor(Def.Rarity));
		CountText->SetText(InvSlot.Count > 1 ? FText::AsNumber(InvSlot.Count) : FText::GetEmpty());

		// Rendered 3D icon (cached); falls back to just the rarity color when the item has no mesh.
		UTextureRenderTarget2D* Icon = nullptr;
		if (UWorld* World = GetWorld())
		{
			if (UItemIconSubsystem* Icons = World->GetSubsystem<UItemIconSubsystem>())
			{
				Icon = Icons->GetIcon(InvSlot.ItemId);
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

	Op->OnDragCancelled.AddDynamic(this, &UInventorySlotWidget::HandleDragCancelled);
	OutOperation = Op;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation)
	{
		return false;
	}
	UInventorySlotWidget* Source = Cast<UInventorySlotWidget>(InOperation->Payload);
	if (!Source || !Inventory.IsValid() || !Source->GetInventory())
	{
		return false;
	}

	UInventoryComponent* SourceInv = Source->GetInventory();
	if (SourceInv == Inventory.Get())
	{
		// Same container: reorder / swap / merge.
		Inventory->MoveItem(Source->GetSlotIndex(), SlotIndex);
	}
	else
	{
		// Cross-container (e.g. chest -> inventory): move as much of the source stack as fits.
		const FInventorySlot& Src = SourceInv->GetSlot(Source->GetSlotIndex());
		if (!Src.IsEmpty())
		{
			const int32 Leftover = Inventory->AddItem(Src.ItemId, Src.Count);
			const int32 Taken = Src.Count - Leftover;
			if (Taken > 0)
			{
				SourceInv->RemoveAt(Source->GetSlotIndex(), Taken);
			}
		}
	}
	return true;
}

void UInventorySlotWidget::HandleDragCancelled(UDragDropOperation* /*Operation*/)
{
	// Dropped on empty space: drop this slot's item into the world as a pickup.
	if (!Inventory.IsValid() || !Inventory->GetSlots().IsValidIndex(SlotIndex))
	{
		return;
	}
	const FInventorySlot& InvSlot = Inventory->GetSlot(SlotIndex);
	if (InvSlot.IsEmpty())
	{
		return;
	}

	APawn* Pawn = GetOwningPlayerPawn();
	UWorld* World = GetWorld();
	if (!Pawn || !World)
	{
		return;
	}

	const FVector Drop = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 180.f;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FTransform(Drop), P))
	{
		Pickup->Configure(InvSlot.ItemId, InvSlot.Count);
		Inventory->RemoveAt(SlotIndex, InvSlot.Count);
	}
}
