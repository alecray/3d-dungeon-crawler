#include "LootSlotWidget.h"
#include "LootChest.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

bool ULootSlotWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UButton>(WidgetTree->RootWidget))
	{
		return true;
	}

	Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button"));
	WidgetTree->RootWidget = Button;
	Button->OnClicked.AddDynamic(this, &ULootSlotWidget::HandleClicked);

	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
	Button->AddChild(Label);

	return true;
}

void ULootSlotWidget::Setup(ALootChest* InChest, int32 InIndex, UInventoryComponent* InInventory)
{
	Chest = InChest;
	Index = InIndex;
	Inventory = InInventory;

	if (!Label || !Chest.IsValid() || !Chest->GetContents().IsValidIndex(Index))
	{
		return;
	}
	const FInventorySlot& InvSlot = Chest->GetContents()[Index];
	const FItemDef& Def = ItemDatabase::Get(InvSlot.ItemId);
	Label->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d"), *Def.DisplayName, InvSlot.Count)));
	Label->SetColorAndOpacity(FSlateColor(RarityColor(Def.Rarity)));
}

void ULootSlotWidget::HandleClicked()
{
	if (Chest.IsValid() && Inventory.IsValid())
	{
		Chest->TakeItem(Index, Inventory.Get());
	}
}
