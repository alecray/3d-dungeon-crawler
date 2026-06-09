#include "MapSelectWidget.h"
#include "DungeonGameInstance.h"
#include "DungeonPlayerController.h"
#include "FirstPersonCharacter.h"

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
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"

// Tier names shown on the buttons and in the label.
static const TCHAR* TierNames[4]   = { TEXT("Normal"), TEXT("Hard"), TEXT("Elite"), TEXT("Nightmare") };
static const TCHAR* TierDescs[4]   = {
	TEXT("Tier 0  –  Normal"),
	TEXT("Tier 1  –  Hard"),
	TEXT("Tier 2  –  Elite"),
	TEXT("Tier 3  –  Nightmare"),
};

bool UMapSelectWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget)) { return true; }

	// ---- Root canvas ----
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Full-screen dim.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.65f));
	if (UCanvasPanelSlot* DS = Root->AddChildToCanvas(Dim))
	{
		DS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DS->SetOffsets(FMargin(0.f));
	}

	// Centred panel.
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.97f));
	Panel->SetPadding(FMargin(28.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetAutoSize(true);
	}

	// Fixed-width inner sizer — wide enough so all four tier buttons breathe.
	USizeBox* PanelSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizer"));
	PanelSizer->SetWidthOverride(620.f);
	Panel->SetContent(PanelSizer);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	PanelSizer->AddChild(Box);

	// ---- Title ----
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Enter Dungeon")));
	Title->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo Font = Title->GetFont();
		Font.Size = 26;
		Title->SetFont(Font);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Title)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
	}

	// ---- Destination section ----
	UTextBlock* MapHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapHeader"));
	MapHeader->SetText(FText::FromString(TEXT("Destination")));
	{
		FSlateFontInfo Font = MapHeader->GetFont();
		Font.Size = 13;
		MapHeader->SetFont(Font);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(MapHeader)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UBorder* MapEntry = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MapEntry"));
	MapEntry->SetBrushColor(FLinearColor(0.12f, 0.20f, 0.30f, 0.90f));
	MapEntry->SetPadding(FMargin(12.f, 8.f));
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(MapEntry)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	MapNameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapName"));
	MapNameLabel->SetText(FText::FromString(TEXT("The Dungeon")));
	{
		FSlateFontInfo Font = MapNameLabel->GetFont();
		Font.Size = 15;
		MapNameLabel->SetFont(Font);
	}
	MapEntry->SetContent(MapNameLabel);

	// ---- Difficulty tier section ----
	UTextBlock* TierHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TierHeader"));
	TierHeader->SetText(FText::FromString(TEXT("Difficulty Tier")));
	{
		FSlateFontInfo Font = TierHeader->GetFont();
		Font.Size = 13;
		TierHeader->SetFont(Font);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(TierHeader)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	// Row of four tier-select buttons — each slot fills equally so they span the full width.
	UHorizontalBox* TierRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TierRow"));
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(TierRow)))
	{
		VS->SetHorizontalAlignment(HAlign_Fill);
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	for (int32 i = 0; i < 4; ++i)
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("TierBtn%d"), i));

		UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("TierBtnText%d"), i));
		BtnText->SetText(FText::FromString(TierNames[i]));
		BtnText->SetJustification(ETextJustify::Center);
		{
			FSlateFontInfo Font = BtnText->GetFont();
			Font.Size = 13;
			BtnText->SetFont(Font);
		}

		// Wrap text in a fixed-height sizer so all buttons are the same height.
		USizeBox* BtnSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("TierBtnSizer%d"), i));
		BtnSizer->SetHeightOverride(48.f);
		if (USizeBoxSlot* TS = Cast<USizeBoxSlot>(BtnSizer->AddChild(BtnText)))
		{
			TS->SetHorizontalAlignment(HAlign_Fill);
			TS->SetVerticalAlignment(VAlign_Center);
		}
		Btn->AddChild(BtnSizer);

		// Fill slots so all four buttons share the row width equally.
		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(TierRow->AddChildToHorizontalBox(Btn)))
		{
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HS->SetPadding(FMargin(i < 3 ? 6.f : 0.f, 0.f, 0.f, 0.f)); // gap between buttons, not after last
			HS->SetVerticalAlignment(VAlign_Fill);
		}

		TierButtons[i] = Btn;
		TierBtnTexts[i] = BtnText;
	}

	// Selected tier description line below the buttons.
	TierLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TierLabel"));
	TierLabel->SetText(FText::FromString(TierDescs[0]));
	TierLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo Font = TierLabel->GetFont();
		Font.Size = 13;
		TierLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(TierLabel)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
		VS->SetPadding(FMargin(0.f, 4.f, 0.f, 24.f));
	}

	// Bind tier button UFUNCTIONs (AddDynamic requires named functions, no lambdas).
	if (TierButtons[0]) { TierButtons[0]->OnClicked.AddDynamic(this, &UMapSelectWidget::OnTier0Clicked); }
	if (TierButtons[1]) { TierButtons[1]->OnClicked.AddDynamic(this, &UMapSelectWidget::OnTier1Clicked); }
	if (TierButtons[2]) { TierButtons[2]->OnClicked.AddDynamic(this, &UMapSelectWidget::OnTier2Clicked); }
	if (TierButtons[3]) { TierButtons[3]->OnClicked.AddDynamic(this, &UMapSelectWidget::OnTier3Clicked); }

	// ---- Go / Cancel row ----
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(ActionRow)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
	}

	auto MakeActionBtn = [&](const TCHAR* Label) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetWidthOverride(160.f);
		BtnSize->SetHeightOverride(42.f);
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		if (USizeBoxSlot* TS = Cast<USizeBoxSlot>(BtnSize->AddChild(T)))
		{
			TS->SetHorizontalAlignment(HAlign_Fill);
			TS->SetVerticalAlignment(VAlign_Center);
		}
		Btn->AddChild(BtnSize);
		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(ActionRow->AddChildToHorizontalBox(Btn)))
		{
			HS->SetPadding(FMargin(8.f, 0.f));
			HS->SetVerticalAlignment(VAlign_Center);
		}
		return Btn;
	};

	UButton* GoBtn     = MakeActionBtn(TEXT("Go"));
	UButton* CancelBtn = MakeActionBtn(TEXT("Cancel"));

	if (GoBtn)     { GoBtn->OnClicked.AddDynamic(this, &UMapSelectWidget::OnGoClicked); }
	if (CancelBtn) { CancelBtn->OnClicked.AddDynamic(this, &UMapSelectWidget::OnCancelClicked); }

	return true;
}

void UMapSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Restore the tier the GameInstance already has, but clamp to whatever is actually unlocked.
	if (UDungeonGameInstance* GI = GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
	{
		const int32 MaxUnlocked = FMath::Clamp(GI->GetHighestTierBeaten() + 1, 0, 3);
		CurrentTier = FMath::Clamp(GI->GetSelectedTier(), 0, MaxUnlocked);
	}
	RefreshTierLabels();
}

void UMapSelectWidget::Init(FName InTargetMap)
{
	TargetMap = InTargetMap;
	if (MapNameLabel && !TargetMap.IsNone())
	{
		// Show a human-readable name: strip "L_" prefix, replace underscores with spaces.
		FString Display = TargetMap.ToString();
		if (Display.StartsWith(TEXT("L_"))) { Display.RemoveFromStart(TEXT("L_")); }
		Display.ReplaceInline(TEXT("_"), TEXT(" "));
		MapNameLabel->SetText(FText::FromString(Display));
	}
}

// ---- Tier helpers ----

void UMapSelectWidget::SelectTier(int32 Tier)
{
	// Enforce progression gate: tier N requires having beaten tier N-1.
	const UDungeonGameInstance* GI = GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	const int32 HighestBeaten = GI ? GI->GetHighestTierBeaten() : -1;
	const int32 MaxUnlocked   = FMath::Clamp(HighestBeaten + 1, 0, 3);

	if (Tier > MaxUnlocked)
	{
		return; // locked
	}
	CurrentTier = FMath::Clamp(Tier, 0, 3);
	RefreshTierLabels();
}

void UMapSelectWidget::RefreshTierLabels()
{
	const UDungeonGameInstance* GI = GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	const int32 HighestBeaten = GI ? GI->GetHighestTierBeaten() : -1;
	const int32 MaxUnlocked   = FMath::Clamp(HighestBeaten + 1, 0, 3);

	if (TierLabel)
	{
		TierLabel->SetText(FText::FromString(TierDescs[CurrentTier]));
	}

	for (int32 i = 0; i < 4; ++i)
	{
		if (!TierButtons[i]) { continue; }

		const bool bLocked   = (i > MaxUnlocked);
		const bool bSelected = (i == CurrentTier);

		FLinearColor Tint;
		if (bLocked)
		{
			Tint = FLinearColor(0.5f, 0.5f, 0.5f, 0.30f); // greyed out
		}
		else if (bSelected)
		{
			Tint = FLinearColor(0.20f, 0.60f, 1.00f, 1.f); // cyan highlight
		}
		else
		{
			Tint = FLinearColor(1.f, 1.f, 1.f, 0.55f); // unselected
		}
		TierButtons[i]->SetColorAndOpacity(Tint);
		TierButtons[i]->SetIsEnabled(!bLocked);

		if (TierBtnTexts[i])
		{
			const FString Label = bLocked
				? FString::Printf(TEXT("%s\n(Locked)"), TierNames[i])
				: FString(TierNames[i]);
			TierBtnTexts[i]->SetText(FText::FromString(Label));
		}
	}
}

// ---- Tier button UFUNCTIONs ----
void UMapSelectWidget::OnTier0Clicked() { SelectTier(0); }
void UMapSelectWidget::OnTier1Clicked() { SelectTier(1); }
void UMapSelectWidget::OnTier2Clicked() { SelectTier(2); }
void UMapSelectWidget::OnTier3Clicked() { SelectTier(3); }

// ---- Action buttons ----

void UMapSelectWidget::OnGoClicked()
{
	const FName MapToTravel = TargetMap.IsNone() ? FName(TEXT("L_DungeonTest")) : TargetMap;

	UDungeonGameInstance* GI = GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	if (GI)
	{
		GI->SetSelectedTier(CurrentTier);
		GI->SetSelectedMap(MapToTravel);
	}

	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		Player->SaveNow();
	}

	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->CloseMapSelectMenu();
		PC->FadeToBlackAndTravel(MapToTravel);
	}
}

void UMapSelectWidget::OnCancelClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->CloseMapSelectMenu();
	}
}
