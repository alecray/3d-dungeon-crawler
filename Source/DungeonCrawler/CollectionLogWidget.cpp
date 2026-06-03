#include "CollectionLogWidget.h"
#include "ItemTypes.h"
#include "DungeonGameInstance.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"

bool UCollectionLogWidget::Initialize()
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
	Panel->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.04f, 0.92f));
	Panel->SetPadding(FMargin(16.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetAutoSize(true);
	}

	List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("List"));
	Panel->AddChild(List);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Collection Log")));
	List->AddChild(Title);

	return true;
}

void UCollectionLogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Rebuild();
}

void UCollectionLogWidget::Rebuild()
{
	if (!List)
	{
		return;
	}

	const UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance());

	for (const FItemDef& Def : ItemDatabase::All())
	{
		// Only rare and above are "collectibles".
		if (Def.Rarity < EItemRarity::Rare)
		{
			continue;
		}

		const bool bFound = GI && GI->IsDiscovered(Def.Id);

		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (bFound)
		{
			Row->SetText(FText::FromString(Def.DisplayName));
			Row->SetColorAndOpacity(FSlateColor(RarityColor(Def.Rarity)));
		}
		else
		{
			Row->SetText(FText::FromString(TEXT("??? (undiscovered)")));
			Row->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f)));
		}
		List->AddChild(Row);
	}
}
