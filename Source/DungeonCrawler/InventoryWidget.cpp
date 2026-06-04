#include "InventoryWidget.h"
#include "InventorySlotWidget.h"
#include "EquipmentSlotWidget.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

	// Horizontal row: inventory column on the left, optional equipment paperdoll on the right.
	Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Panel->AddChild(Row);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UHorizontalBoxSlot* CS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(Column)))
	{
		CS->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetText(FText::FromString(TEXT("Inventory")));
	Column->AddChild(TitleText);

	// Gold readout (only shown on the player's own inventory, not the chest loot grid).
	GoldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoldText"));
	GoldText->SetText(FText::FromString(TEXT("Gold: 0")));
	GoldText->SetVisibility(ESlateVisibility::Collapsed);
	Column->AddChild(GoldText);

	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
	Grid->SetSlotPadding(FMargin(3.f));
	Column->AddChild(Grid);

	return true;
}

void UInventoryWidget::SetShowEquipment(bool bShow)
{
	bShowEquipment = bShow;
	if (bShow && !EquipSection)
	{
		BuildEquipmentPanel();
	}
	if (EquipSection)
	{
		EquipSection->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	RefreshEquipment();
	RefreshGold();
}

void UInventoryWidget::BuildEquipmentPanel()
{
	if (!Row || EquipSection)
	{
		return;
	}

	UVerticalBox* EquipColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EquipColumn"));
	EquipSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EquipSection"));
	EquipSection->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f)); // transparent wrapper
	EquipSection->SetContent(EquipColumn);
	Row->AddChildToHorizontalBox(EquipSection);

	UTextBlock* EquipTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EquipTitle"));
	EquipTitle->SetText(FText::FromString(TEXT("Equipment")));
	EquipColumn->AddChild(EquipTitle);

	EquipCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EquipCanvas"));
	EquipColumn->AddChild(EquipCanvas);

	AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn());
	UEquipmentComponent* Equip = Player ? Player->GetEquipmentComponent() : nullptr;
	if (!Equip)
	{
		return;
	}

	// Paperdoll layout (canvas pixel positions, 58px slots) matching the mockup: a center column with
	// amulet+gloves on the left and the four rings stacked on the right.
	struct FSlotPos { int32 Slot; float X; float Y; };
	static const FSlotPos Layout[] = {
		{ UEquipmentComponent::Head,   130.f,   0.f },
		{ UEquipmentComponent::Body,   130.f,  68.f },
		{ UEquipmentComponent::Belt,   130.f, 136.f },
		{ UEquipmentComponent::Legs,   130.f, 204.f },
		{ UEquipmentComponent::Feet,   130.f, 272.f },
		{ UEquipmentComponent::Amulet,  62.f,  34.f },
		{ UEquipmentComponent::Gloves,  62.f, 136.f },
		{ UEquipmentComponent::Ring1,  198.f,  68.f },
		{ UEquipmentComponent::Ring2,  266.f,  68.f },
		{ UEquipmentComponent::Ring3,  198.f, 136.f },
		{ UEquipmentComponent::Ring4,  266.f, 136.f },
	};

	EquipSlotWidgets.Reset();
	for (const FSlotPos& Entry : Layout)
	{
		UEquipmentSlotWidget* SlotW = CreateWidget<UEquipmentSlotWidget>(this, UEquipmentSlotWidget::StaticClass());
		if (!SlotW)
		{
			continue;
		}
		SlotW->Setup(Equip, Entry.Slot);
		if (UCanvasPanelSlot* CPS = EquipCanvas->AddChildToCanvas(SlotW))
		{
			CPS->SetAutoSize(true);
			CPS->SetPosition(FVector2D(Entry.X, Entry.Y));
		}
		EquipSlotWidgets.Add(SlotW);
	}

	// Refresh the paperdoll whenever equipment changes (equip/unequip moves items to/from the bag).
	Equip->OnEquipmentChanged.AddUObject(this, &UInventoryWidget::RefreshEquipment);
}

void UInventoryWidget::RefreshEquipment()
{
	for (UEquipmentSlotWidget* SlotW : EquipSlotWidgets)
	{
		if (SlotW) { SlotW->Refresh(); }
	}
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
	RefreshGold();
}

void UInventoryWidget::NativeDestruct()
{
	if (Inventory.IsValid())
	{
		Inventory->OnInventoryChanged.RemoveAll(this);
	}
	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		if (UEquipmentComponent* Equip = Player->GetEquipmentComponent())
		{
			Equip->OnEquipmentChanged.RemoveAll(this);
		}
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
	RefreshGold();
}

void UInventoryWidget::RefreshGold()
{
	if (!GoldText)
	{
		return;
	}
	// Gold only makes sense on the player's own inventory screen (the chest loot grid reuses this widget).
	if (!bShowEquipment)
	{
		GoldText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	GoldText->SetVisibility(ESlateVisibility::Visible);
	const int32 Gold = (Cast<AFirstPersonCharacter>(GetOwningPlayerPawn())) ? Cast<AFirstPersonCharacter>(GetOwningPlayerPawn())->GetGold() : 0;
	GoldText->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), Gold)));
}
