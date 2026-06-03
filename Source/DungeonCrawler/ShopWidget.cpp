#include "ShopWidget.h"
#include "ItemTypes.h"
#include "InventoryComponent.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

static int32 BuyPrice(FName ItemId)
{
	return ItemDatabase::Contains(ItemId) ? FMath::Max(1, ItemDatabase::Get(ItemId).Value) : 1;
}

static int32 SellPrice(FName ItemId)
{
	return FMath::Max(1, BuyPrice(ItemId) / 2);
}

void UShopClickProxy::OnClicked()
{
	if (!Owner.IsValid())
	{
		return;
	}
	if (bSell) { Owner->Sell(SlotIndex); }
	else       { Owner->Buy(ItemId); }
}

bool UShopWidget::Initialize()
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

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.06f, 0.94f));
	Panel->SetPadding(FMargin(18.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetSize(FVector2D(720.f, 520.f));
	}

	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Outer"));
	Panel->SetContent(Outer);

	GoldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoldText"));
	GoldText->SetText(FText::FromString(TEXT("Shop")));
	Outer->AddChildToVerticalBox(GoldText);

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Columns"));
	if (UVerticalBoxSlot* CS = Cast<UVerticalBoxSlot>(Outer->AddChildToVerticalBox(Columns)))
	{
		CS->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}

	auto MakeColumn = [&](const TCHAR* Heading) -> UVerticalBox*
	{
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(Columns->AddChildToHorizontalBox(Col)))
		{
			HS->SetPadding(FMargin(10.f, 0.f));
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UTextBlock* H = WidgetTree->ConstructWidget<UTextBlock>();
		H->SetText(FText::FromString(Heading));
		H->SetJustification(ETextJustify::Center);
		Col->AddChildToVerticalBox(H);
		return Col;
	};

	BuyList = MakeColumn(TEXT("Buy"));
	SellList = MakeColumn(TEXT("Sell  (half value)"));

	return true;
}

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UInventoryComponent* Inv = GetInventory())
	{
		InventoryChangedHandle = Inv->OnInventoryChanged.AddUObject(this, &UShopWidget::HandleInventoryChanged);
	}
	Refresh();
}

void UShopWidget::NativeDestruct()
{
	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->OnInventoryChanged.Remove(InventoryChangedHandle);
	}
	Super::NativeDestruct();
}

void UShopWidget::HandleInventoryChanged(UInventoryComponent*)
{
	Refresh();
}

void UShopWidget::Buy(FName ItemId)
{
	AFirstPersonCharacter* Player = GetPlayer();
	UInventoryComponent* Inv = GetInventory();
	if (!Player || !Inv)
	{
		return;
	}
	const int32 Price = BuyPrice(ItemId);
	if (!Player->TrySpendGold(Price))
	{
		return; // not enough gold
	}
	const int32 Leftover = Inv->AddItem(ItemId, 1);
	if (Leftover > 0)
	{
		Player->AddGold(Price); // inventory full: refund
	}
	Refresh();
}

void UShopWidget::Sell(int32 SlotIndex)
{
	AFirstPersonCharacter* Player = GetPlayer();
	UInventoryComponent* Inv = GetInventory();
	if (!Player || !Inv || SlotIndex < 0 || SlotIndex >= Inv->NumSlots())
	{
		return;
	}
	const FInventorySlot& InvSlot = Inv->GetSlot(SlotIndex);
	if (InvSlot.IsEmpty())
	{
		return;
	}
	const int32 Price = SellPrice(InvSlot.ItemId);
	Inv->RemoveAt(SlotIndex, 1);
	Player->AddGold(Price);
	Refresh();
}

void UShopWidget::Refresh()
{
	AFirstPersonCharacter* Player = GetPlayer();
	UInventoryComponent* Inv = GetInventory();
	const int32 Gold = Player ? Player->GetGold() : 0;

	if (GoldText)
	{
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("Shop      Gold: %d"), Gold)));
	}

	// Rebuild both columns from scratch (entries depend on current gold + inventory contents).
	Proxies.Reset();
	auto AddRow = [&](UVerticalBox* Column, const FString& Label, bool bEnabled, bool bSell, FName ItemId, int32 SlotIndex)
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Btn->AddChild(Text);
		Btn->SetIsEnabled(bEnabled);
		if (UVerticalBoxSlot* BS = Cast<UVerticalBoxSlot>(Column->AddChildToVerticalBox(Btn)))
		{
			BS->SetPadding(FMargin(0.f, 4.f));
		}

		UShopClickProxy* Proxy = NewObject<UShopClickProxy>(this);
		Proxy->bSell = bSell;
		Proxy->ItemId = ItemId;
		Proxy->SlotIndex = SlotIndex;
		Proxy->Owner = this;
		Btn->OnClicked.AddDynamic(Proxy, &UShopClickProxy::OnClicked);
		Proxies.Add(Proxy);
	};

	if (BuyList)
	{
		// Clear all but the heading (index 0).
		while (BuyList->GetChildrenCount() > 1) { BuyList->RemoveChildAt(BuyList->GetChildrenCount() - 1); }
		for (const FName& Ware : Wares)
		{
			if (!ItemDatabase::Contains(Ware)) { continue; }
			const FItemDef& Def = ItemDatabase::Get(Ware);
			const int32 Price = BuyPrice(Ware);
			AddRow(BuyList, FString::Printf(TEXT("%s  -  %dg"), *Def.DisplayName, Price),
				Gold >= Price, /*bSell*/ false, Ware, -1);
		}
	}

	if (SellList && Inv)
	{
		while (SellList->GetChildrenCount() > 1) { SellList->RemoveChildAt(SellList->GetChildrenCount() - 1); }
		for (int32 i = 0; i < Inv->NumSlots(); ++i)
		{
			const FInventorySlot& InvSlot = Inv->GetSlot(i);
			if (InvSlot.IsEmpty() || !ItemDatabase::Contains(InvSlot.ItemId)) { continue; }
			const FItemDef& Def = ItemDatabase::Get(InvSlot.ItemId);
			AddRow(SellList, FString::Printf(TEXT("%s x%d  -  %dg"), *Def.DisplayName, InvSlot.Count, SellPrice(InvSlot.ItemId)),
				true, /*bSell*/ true, InvSlot.ItemId, i);
		}
	}
}

AFirstPersonCharacter* UShopWidget::GetPlayer() const
{
	return Cast<AFirstPersonCharacter>(GetOwningPlayerPawn());
}

UInventoryComponent* UShopWidget::GetInventory() const
{
	AFirstPersonCharacter* Player = GetPlayer();
	return Player ? Player->GetInventoryComponent() : nullptr;
}
