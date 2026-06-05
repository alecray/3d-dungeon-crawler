#include "BlackjackWidget.h"
#include "BlackjackTable.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

bool UBlackjackWidget::Initialize()
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

	// A control panel pinned to the left-center of the screen (cards are shown in 3D on the table).
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.03f, 0.10f, 0.05f, 0.94f));
	Panel->SetPadding(FMargin(18.f, 16.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.f, 0.5f));
		PS->SetAlignment(FVector2D(0.f, 0.5f));
		PS->SetPosition(FVector2D(40.f, 0.f));
		PS->SetSize(FVector2D(260.f, 0.f)); // fixed width so the stacked button rows line up; height auto
		PS->SetAutoSize(false);
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Panel->SetContent(Box);

	auto AddLabel = [&](const TCHAR* Initial, int32 FontSize, float TopPad) -> UTextBlock*
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Initial));
		T->SetJustification(ETextJustify::Center);
		if (FSlateFontInfo F = T->GetFont(); true) { F.Size = FontSize; T->SetFont(F); }
		if (UVerticalBoxSlot* S = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(T)))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.f, TopPad, 0.f, 0.f));
		}
		return T;
	};

	// New vertical row of evenly-filled buttons; each button stretches to share the row width equally.
	auto AddButtonRow = [&](float TopPad) -> UHorizontalBox*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* RS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Row)))
		{
			RS->SetHorizontalAlignment(HAlign_Fill);
			RS->SetPadding(FMargin(0.f, TopPad, 0.f, 0.f));
		}
		return Row;
	};

	GoldText = AddLabel(TEXT("Gold: 0"), 20, 0.f);   // top of the left panel
	BetText  = AddLabel(TEXT("Bet: 25"), 18, 6.f);   // (Dealer/You values are 3D labels on the table now)

	UHorizontalBox* BetRow = AddButtonRow(12.f);
	BetDownBtn = MakeButton(BetRow, TEXT("Bet -"), &UBlackjackWidget::OnBetDown);
	BetUpBtn   = MakeButton(BetRow, TEXT("Bet +"), &UBlackjackWidget::OnBetUp);

	UHorizontalBox* ActRow = AddButtonRow(8.f);
	DealBtn  = MakeButton(ActRow, TEXT("Deal"),  &UBlackjackWidget::OnDeal);
	HitBtn   = MakeButton(ActRow, TEXT("Hit"),   &UBlackjackWidget::OnHit);
	StandBtn = MakeButton(ActRow, TEXT("Stand"), &UBlackjackWidget::OnStand);

	UHorizontalBox* LeaveRow = AddButtonRow(8.f);
	MakeButton(LeaveRow, TEXT("Leave"), &UBlackjackWidget::OnLeave);

	// Status line ("Hit or Stand." / result) pinned to the bottom-center of the screen, above the hotbar.
	StatusText = WidgetTree->ConstructWidget<UTextBlock>();
	StatusText->SetText(FText::FromString(TEXT("Blackjack")));
	StatusText->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo F = StatusText->GetFont(); true) { F.Size = 22; StatusText->SetFont(F); }
	if (UCanvasPanelSlot* SS = Root->AddChildToCanvas(StatusText))
	{
		SS->SetAnchors(FAnchors(0.5f, 1.f));
		SS->SetAlignment(FVector2D(0.5f, 1.f));
		SS->SetPosition(FVector2D(0.f, -110.f)); // sit above the hotbar at the bottom-center
		SS->SetAutoSize(true);
	}

	return true;
}

UButton* UBlackjackWidget::MakeButton(UHorizontalBox* Row, const TCHAR* Label, void (UBlackjackWidget::*Handler)())
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Label));
	T->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo F = T->GetFont(); true) { F.Size = 18; T->SetFont(F); }
	Btn->AddChild(T); // UButtonSlot centers its content by default, so the label sits centered
	if (UHorizontalBoxSlot* S = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(Btn)))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); // every button shares its row width equally
		S->SetPadding(FMargin(4.f, 0.f));
	}
	if (Handler == &UBlackjackWidget::OnBetDown) { Btn->OnClicked.AddDynamic(this, &UBlackjackWidget::OnBetDown); }
	else if (Handler == &UBlackjackWidget::OnBetUp) { Btn->OnClicked.AddDynamic(this, &UBlackjackWidget::OnBetUp); }
	else if (Handler == &UBlackjackWidget::OnDeal) { Btn->OnClicked.AddDynamic(this, &UBlackjackWidget::OnDeal); }
	else if (Handler == &UBlackjackWidget::OnHit) { Btn->OnClicked.AddDynamic(this, &UBlackjackWidget::OnHit); }
	else if (Handler == &UBlackjackWidget::OnStand) { Btn->OnClicked.AddDynamic(this, &UBlackjackWidget::OnStand); }
	else { Btn->OnClicked.AddDynamic(this, &UBlackjackWidget::OnLeave); }
	return Btn;
}

void UBlackjackWidget::OpenFor(ABlackjackTable* InTable, APlayerController* PC)
{
	Table = InTable;
	if (!IsInViewport()) { AddToViewport(50); }
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
	RefreshFromTable();
}

void UBlackjackWidget::RefreshFromTable()
{
	ABlackjackTable* T = Table.Get();
	if (!T) { return; }

	if (StatusText) { StatusText->SetText(FText::FromString(T->GetStatusLine())); }
	if (GoldText) { GoldText->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), T->GetGold()))); }
	if (BetText) { BetText->SetText(FText::FromString(FString::Printf(TEXT("Bet: %d"), T->GetBet()))); }

	const bool bBet = T->CanBet();
	const bool bPlay = T->CanHitStand();
	if (BetDownBtn) { BetDownBtn->SetIsEnabled(bBet); }
	if (BetUpBtn)   { BetUpBtn->SetIsEnabled(bBet); }
	if (DealBtn)    { DealBtn->SetIsEnabled(bBet); }
	if (HitBtn)     { HitBtn->SetIsEnabled(bPlay); }
	if (StandBtn)   { StandBtn->SetIsEnabled(bPlay); }
}

void UBlackjackWidget::OnBetDown() { if (ABlackjackTable* T = Table.Get()) { T->BetDown(); } }
void UBlackjackWidget::OnBetUp()   { if (ABlackjackTable* T = Table.Get()) { T->BetUp(); } }
void UBlackjackWidget::OnDeal()    { if (ABlackjackTable* T = Table.Get()) { T->Deal(); } }
void UBlackjackWidget::OnHit()     { if (ABlackjackTable* T = Table.Get()) { T->Hit(); } }
void UBlackjackWidget::OnStand()   { if (ABlackjackTable* T = Table.Get()) { T->Stand(); } }
void UBlackjackWidget::OnLeave()   { if (ABlackjackTable* T = Table.Get()) { T->Leave(); } }

void UBlackjackWidget::CloseBar()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	RemoveFromParent();
}
