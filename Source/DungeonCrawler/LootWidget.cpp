#include "LootWidget.h"
#include "LootSlotWidget.h"
#include "LootChest.h"
#include "InventoryComponent.h"
#include "FirstPersonCharacter.h"
#include "DungeonPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

namespace
{
	UButton* MakeTextButton(UWidgetTree* Tree, const TCHAR* Text)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Text));
		B->AddChild(T);
		return B;
	}
}

bool ULootWidget::Initialize()
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
	Panel->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.04f, 0.94f));
	Panel->SetPadding(FMargin(16.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D(-360.f, 0.f)); // sit left of center; inventory can open at center
		PS->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->AddChild(Column);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Chest")));
	Column->AddChild(Title);

	List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("List"));
	Column->AddChild(List);

	TakeAllButton = MakeTextButton(WidgetTree, TEXT("Take All"));
	TakeAllButton->OnClicked.AddDynamic(this, &ULootWidget::OnTakeAllClicked);
	Column->AddChild(TakeAllButton);

	CloseButton = MakeTextButton(WidgetTree, TEXT("Close"));
	CloseButton->OnClicked.AddDynamic(this, &ULootWidget::OnCloseClicked);
	Column->AddChild(CloseButton);

	return true;
}

void ULootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		Inventory = Player->GetInventoryComponent();
	}
	Rebuild();
}

void ULootWidget::NativeDestruct()
{
	if (Chest.IsValid())
	{
		Chest->OnContentsChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void ULootWidget::SetChest(ALootChest* InChest)
{
	if (Chest.IsValid())
	{
		Chest->OnContentsChanged.RemoveAll(this);
	}
	Chest = InChest;
	if (Chest.IsValid())
	{
		Chest->OnContentsChanged.AddUObject(this, &ULootWidget::Rebuild);
	}
	Rebuild();
}

void ULootWidget::Rebuild(ALootChest* /*Changed*/)
{
	if (!List)
	{
		return;
	}
	List->ClearChildren();

	if (!Chest.IsValid())
	{
		return;
	}

	const TArray<FInventorySlot>& Contents = Chest->GetContents();
	bool bAny = false;
	for (int32 i = 0; i < Contents.Num(); ++i)
	{
		if (Contents[i].IsEmpty())
		{
			continue;
		}
		bAny = true;
		ULootSlotWidget* Row = CreateWidget<ULootSlotWidget>(this, ULootSlotWidget::StaticClass());
		if (Row)
		{
			Row->Setup(Chest.Get(), i, Inventory.Get());
			List->AddChild(Row);
		}
	}

	if (!bAny)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Empty->SetText(FText::FromString(TEXT("(empty)")));
		List->AddChild(Empty);
	}
}

void ULootWidget::OnTakeAllClicked()
{
	if (Chest.IsValid() && Inventory.IsValid())
	{
		Chest->TakeAll(Inventory.Get());
	}
}

void ULootWidget::OnCloseClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->CloseLootMenu();
	}
}
