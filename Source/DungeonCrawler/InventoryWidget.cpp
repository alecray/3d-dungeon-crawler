#include "InventoryWidget.h"
#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"

bool UInventoryWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		return true;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.04f, 0.92f));
	Panel->SetPadding(FMargin(16.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(PanelPosition);
		PS->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->AddChild(Column);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetText(FText::FromString(TEXT("Inventory")));
	Column->AddChild(TitleText);

	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
	Grid->SetSlotPadding(FMargin(3.f));
	Column->AddChild(Grid);

	return true;
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// If nobody assigned an inventory, default to the owning player's.
	if (!bInventorySet)
	{
		if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
		{
			SetInventory(Player->GetInventoryComponent(), TEXT("Inventory"));
		}
	}
}

void UInventoryWidget::NativeDestruct()
{
	if (Inventory.IsValid())
	{
		Inventory->OnInventoryChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UInventoryWidget::SetInventory(UInventoryComponent* InInventory, const FString& Title)
{
	if (Inventory.IsValid())
	{
		Inventory->OnInventoryChanged.RemoveAll(this);
	}

	Inventory = InInventory;
	bInventorySet = true;

	if (TitleText)
	{
		TitleText->SetText(FText::FromString(Title));
	}

	RebuildGrid();

	if (Inventory.IsValid())
	{
		Inventory->OnInventoryChanged.AddUObject(this, &UInventoryWidget::RefreshAll);
		RefreshAll();
	}
}

void UInventoryWidget::SetPanelPosition(FVector2D Pos)
{
	PanelPosition = Pos;
	if (Panel)
	{
		if (UCanvasPanelSlot* PS = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			PS->SetPosition(Pos);
		}
	}
}

void UInventoryWidget::RebuildGrid()
{
	if (!Grid)
	{
		return;
	}
	Grid->ClearChildren();
	SlotWidgets.Reset();

	if (!Inventory.IsValid())
	{
		return;
	}

	const int32 Count = Inventory->NumSlots();
	for (int32 i = 0; i < Count; ++i)
	{
		UInventorySlotWidget* SlotW = CreateWidget<UInventorySlotWidget>(this, UInventorySlotWidget::StaticClass());
		if (!SlotW)
		{
			continue;
		}
		SlotW->Setup(Inventory.Get(), i);
		Grid->AddChildToUniformGrid(SlotW, i / Columns, i % Columns);
		SlotWidgets.Add(SlotW);
	}
}

void UInventoryWidget::RefreshAll(UInventoryComponent* /*Changed*/)
{
	for (UInventorySlotWidget* SlotW : SlotWidgets)
	{
		if (SlotW)
		{
			SlotW->Refresh();
		}
	}
}
