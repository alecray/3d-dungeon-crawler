#include "StatsScreenWidget.h"
#include "DungeonGameInstance.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

bool UStatsScreenWidget::Initialize()
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

	// Opaque backdrop so the menu/game behind doesn't bleed through.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 1.f));
	if (UCanvasPanelSlot* BS = Root->AddChildToCanvas(Backdrop))
	{
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Backdrop->SetContent(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("STATS")));
	Title->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo F = Title->GetFont(); true) { F.Size = 40; Title->SetFont(F); }
	if (UVerticalBoxSlot* TS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Title)))
	{
		TS->SetHorizontalAlignment(HAlign_Center);
		TS->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	}

	// Fixed-height scroll area so a growing stat list never runs off-screen.
	USizeBox* ScrollSize = WidgetTree->ConstructWidget<USizeBox>();
	ScrollSize->SetWidthOverride(520.f);
	ScrollSize->SetHeightOverride(420.f);
	if (UVerticalBoxSlot* SS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(ScrollSize)))
	{
		SS->SetHorizontalAlignment(HAlign_Center);
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	ScrollSize->AddChild(Scroll);

	RowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowsBox"));
	Scroll->AddChild(RowsBox);

	UButton* Back = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>();
	BackText->SetText(FText::FromString(TEXT("  Back  ")));
	BackText->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo F = BackText->GetFont(); true) { F.Size = 22; BackText->SetFont(F); }
	Back->AddChild(BackText);
	Back->OnClicked.AddDynamic(this, &UStatsScreenWidget::OnBackClicked);
	if (UVerticalBoxSlot* BkS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Back)))
	{
		BkS->SetHorizontalAlignment(HAlign_Center);
		BkS->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	}

	return true;
}

void UStatsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Populate();
}

void UStatsScreenWidget::Populate()
{
	if (!RowsBox)
	{
		return;
	}
	RowsBox->ClearChildren();

	UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}
	const FPlayerStats& S = GI->GetStats();

	// "Label .......... value" row.
	auto AddRow = [&](const TCHAR* Label, const FString& Value)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		RowsBox->AddChildToVerticalBox(Row);

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(FText::FromString(Label));
		if (FSlateFontInfo F = Name->GetFont(); true) { F.Size = 20; Name->SetFont(F); }
		if (UHorizontalBoxSlot* NS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(Name)))
		{
			NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NS->SetPadding(FMargin(4.f, 5.f));
		}

		UTextBlock* Val = WidgetTree->ConstructWidget<UTextBlock>();
		Val->SetText(FText::FromString(Value));
		Val->SetJustification(ETextJustify::Right);
		if (FSlateFontInfo F = Val->GetFont(); true) { F.Size = 20; Val->SetFont(F); }
		Val->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.86f, 0.3f)));
		if (UHorizontalBoxSlot* VS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(Val)))
		{
			VS->SetPadding(FMargin(16.f, 5.f, 4.f, 5.f));
		}
	};

	auto Num = [](int32 N) { return FText::AsNumber(N).ToString(); };

	AddRow(TEXT("Monsters Killed"), Num(S.MonstersKilled));
	AddRow(TEXT("Bosses Killed"),   Num(S.BossesKilled));
	AddRow(TEXT("Deaths"),            Num(S.Deaths));
	AddRow(TEXT("Death Scrolls Used"), Num(S.DeathScrollsUsed));
	AddRow(TEXT("Gold Looted"),     Num(S.GoldLooted));
	AddRow(TEXT("Chests Opened"),   Num(S.ChestsOpened));
	AddRow(TEXT("Fish Caught"),     Num(S.FishCaught));
	AddRow(TEXT("Blackjack Hands"), Num(S.BlackjackHandsPlayed));
	AddRow(TEXT("  Hands Won"),     Num(S.BlackjackHandsWon));
	AddRow(TEXT("  Hands Lost"),    Num(S.BlackjackHandsLost));
	if (S.BlackjackHandsPlayed > 0)
	{
		const int32 Pct = FMath::RoundToInt(100.f * S.BlackjackHandsWon / S.BlackjackHandsPlayed);
		AddRow(TEXT("  Win Rate"), FString::Printf(TEXT("%d%%"), Pct));
	}
	AddRow(TEXT("Wall Deflects"),   Num(S.DeflectsOffWalls));
}

void UStatsScreenWidget::OnBackClicked()
{
	RemoveFromParent(); // the menu behind us returns
}
