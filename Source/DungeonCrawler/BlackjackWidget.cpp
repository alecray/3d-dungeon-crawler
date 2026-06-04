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

	// A control bar pinned to the bottom-center (the cards/values are shown in 3D on the table).
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.03f, 0.10f, 0.05f, 0.94f));
	Panel->SetPadding(FMargin(20.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 1.f));
		PS->SetAlignment(FVector2D(0.5f, 1.f));
		PS->SetPosition(FVector2D(0.f, -40.f));
		PS->SetAutoSize(true);
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Panel->SetContent(Box);

	auto AddLabel = [&](const TCHAR* Initial, int32 FontSize) -> UTextBlock*
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Initial));
		T->SetJustification(ETextJustify::Center);
		if (FSlateFontInfo F = T->GetFont(); true) { F.Size = FontSize; T->SetFont(F); }
		if (UVerticalBoxSlot* S = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(T)))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.f, 3.f));
		}
		return T;
	};

	StatusText = AddLabel(TEXT("Blackjack"), 20);
	InfoText = AddLabel(TEXT("Gold: 0      Bet: 25"), 18);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	if (UVerticalBoxSlot* RS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Row)))
	{
		RS->SetHorizontalAlignment(HAlign_Center);
		RS->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	BetDownBtn = MakeButton(Row, TEXT("  Bet -  "), &UBlackjackWidget::OnBetDown);
	BetUpBtn   = MakeButton(Row, TEXT("  Bet +  "), &UBlackjackWidget::OnBetUp);
	DealBtn    = MakeButton(Row, TEXT("  Deal  "), &UBlackjackWidget::OnDeal);
	HitBtn     = MakeButton(Row, TEXT("  Hit  "), &UBlackjackWidget::OnHit);
	StandBtn   = MakeButton(Row, TEXT("  Stand  "), &UBlackjackWidget::OnStand);
	MakeButton(Row, TEXT("  Leave  "), &UBlackjackWidget::OnLeave);

	return true;
}

UButton* UBlackjackWidget::MakeButton(UHorizontalBox* Row, const TCHAR* Label, void (UBlackjackWidget::*Handler)())
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Label));
	if (FSlateFontInfo F = T->GetFont(); true) { F.Size = 18; T->SetFont(F); }
	Btn->AddChild(T);
	if (UHorizontalBoxSlot* S = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(Btn)))
	{
		S->SetPadding(FMargin(6.f, 0.f));
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
	if (InfoText) { InfoText->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d        Bet: %d"), T->GetGold(), T->GetBet()))); }

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
