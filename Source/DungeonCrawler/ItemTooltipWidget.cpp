#include "ItemTooltipWidget.h"
#include "ItemTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"

bool UItemTooltipWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UBorder>(WidgetTree->RootWidget))
	{
		return true;
	}

	// Black panel (roomy padding — ~2x the old plain tooltip).
	Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Bg"));
	Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.92f));
	Background->SetPadding(FMargin(16.f, 12.f));
	WidgetTree->RootWidget = Background;

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Background->SetContent(Box);

	auto AddLine = [&](int32 FontSize, const FLinearColor& Color, bool bWrap) -> UTextBlock*
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		if (FSlateFontInfo F = T->GetFont(); true) { F.Size = FontSize; T->SetFont(F); }
		T->SetColorAndOpacity(FSlateColor(Color));
		if (bWrap) { T->SetAutoWrapText(true); T->SetWrapTextAt(380.f); }
		if (UVerticalBoxSlot* S = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(T)))
		{
			S->SetPadding(FMargin(0.f, 3.f));
		}
		return T;
	};

	const FLinearColor Yellow(1.f, 0.85f, 0.15f);
	NameText  = AddLine(26, FLinearColor::White, /*bWrap*/ false); // item name — white
	TypeText  = AddLine(20, Yellow,              /*bWrap*/ false); // "Common Armor" — recolored by rarity in SetItem
	DescText  = AddLine(20, Yellow,              /*bWrap*/ true);  // description — yellow
	ValueText = AddLine(18, Yellow,              /*bWrap*/ false); // value — yellow
	HintText  = AddLine(16, FLinearColor(0.6f, 0.6f, 0.6f), false); // optional dim hint ("(click to unequip)")
	return true;
}

void UItemTooltipWidget::SetItem(FName ItemId, const FString& ExtraLine)
{
	if (!ItemDatabase::Contains(ItemId))
	{
		return;
	}
	const FItemDef& Def = ItemDatabase::Get(ItemId);

	if (NameText) { NameText->SetText(FText::FromString(Def.DisplayName)); }
	if (TypeText)
	{
		TypeText->SetText(FText::FromString(FString::Printf(TEXT("%s %s"), *RarityName(Def.Rarity), *ItemTypeName(Def.Type))));
		TypeText->SetColorAndOpacity(FSlateColor(RarityColor(Def.Rarity))); // armor/type line colored by rarity
	}
	if (DescText)
	{
		DescText->SetText(FText::FromString(Def.Description));
		DescText->SetVisibility(Def.Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ValueText) { ValueText->SetText(FText::FromString(FString::Printf(TEXT("Value: %dg"), Def.Value))); }
	if (HintText)
	{
		HintText->SetText(FText::FromString(ExtraLine));
		HintText->SetVisibility(ExtraLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}
