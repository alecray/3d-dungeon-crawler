#include "MainMenuWidget.h"
#include "DungeonPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"

bool UMainMenuWidget::Initialize()
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

	// Full-screen opaque backdrop (so nothing behind the menu shows through).
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 1.f));
	if (UCanvasPanelSlot* BS = Root->AddChildToCanvas(Backdrop))
	{
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Backdrop->SetContent(Box);
	Box->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("DUNGEON CRAWLER")));
	Title->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo Font = Title->GetFont(); true)
	{
		Font.Size = 48;
		Title->SetFont(Font);
	}
	if (UVerticalBoxSlot* TS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Title)))
	{
		TS->SetHorizontalAlignment(HAlign_Center);
		TS->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));
	}

	auto AddButton = [&](const TCHAR* Label, void (UMainMenuWidget::*Handler)())
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		if (FSlateFontInfo Font = T->GetFont(); true) { Font.Size = 22; T->SetFont(Font); }
		Btn->AddChild(T);
		if (UVerticalBoxSlot* BSlot = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Btn)))
		{
			BSlot->SetHorizontalAlignment(HAlign_Center);
			BSlot->SetPadding(FMargin(0.f, 10.f));
		}
		return Btn;
	};

	AddButton(TEXT("  Start Game  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnStartClicked);
	AddButton(TEXT("  Quit  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);

	return true;
}

void UMainMenuWidget::OnStartClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->StartGameFromMenu();
	}
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}
