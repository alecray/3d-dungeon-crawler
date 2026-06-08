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

bool UMapSelectWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	// Guard against double-init (e.g. if a Blueprint base already built the tree).
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		return true;
	}

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
	Panel->SetPadding(FMargin(24.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetAutoSize(true);
	}

	// Fixed-width inner sizer — same pattern as PauseMenuWidget.
	USizeBox* PanelSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizer"));
	PanelSizer->SetWidthOverride(560.f);
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
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	}

	// ---- Map section header ----
	UTextBlock* MapHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapHeader"));
	MapHeader->SetText(FText::FromString(TEXT("Destination")));
	{
		FSlateFontInfo Font = MapHeader->GetFont();
		Font.Size = 14;
		MapHeader->SetFont(Font);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(MapHeader)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	// ---- Map entry (single destination — the portal's target) ----
	// Displayed as a static label for now; future iterations can turn this into a scrollable list.
	UBorder* MapEntry = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MapEntry"));
	MapEntry->SetBrushColor(FLinearColor(0.12f, 0.20f, 0.30f, 0.90f));
	MapEntry->SetPadding(FMargin(10.f, 8.f));
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(MapEntry)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
	}

	MapNameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapName"));
	// Placeholder until Init() supplies the real target map name.
	MapNameLabel->SetText(FText::FromString(TEXT("The Dungeon")));
	MapEntry->SetContent(MapNameLabel);

	// ---- Tier section ----
	UTextBlock* TierHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TierHeader"));
	TierHeader->SetText(FText::FromString(TEXT("Difficulty Tier")));
	{
		FSlateFontInfo Font = TierHeader->GetFont();
		Font.Size = 14;
		TierHeader->SetFont(Font);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(TierHeader)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	// Current tier display.
	TierLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TierLabel"));
	TierLabel->SetText(FText::FromString(TEXT("Tier: 0 (Normal)")));
	TierLabel->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(TierLabel)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	// Row of four tier-select buttons.
	UHorizontalBox* TierRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TierRow"));
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(TierRow)))
	{
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	static const TCHAR* TierNames[4] = { TEXT("0\nNormal"), TEXT("1\nHard"), TEXT("2\nElite"), TEXT("3\nNightmare") };

	for (int32 i = 0; i < 4; ++i)
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("TierBtn%d"), i));

		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("TierBtnSize%d"), i));
		BtnSize->SetWidthOverride(120.f);
		BtnSize->SetHeightOverride(52.f);

		UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("TierBtnText%d"), i));
		BtnText->SetText(FText::FromString(TierNames[i]));
		BtnText->SetJustification(ETextJustify::Center);
		if (USizeBoxSlot* TS = Cast<USizeBoxSlot>(BtnSize->AddChild(BtnText)))
		{
			TS->SetHorizontalAlignment(HAlign_Fill);
			TS->SetVerticalAlignment(VAlign_Center);
		}
		Btn->AddChild(BtnSize);

		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(TierRow->AddChildToHorizontalBox(Btn)))
		{
			HS->SetPadding(FMargin(4.f, 0.f));
			HS->SetVerticalAlignment(VAlign_Center);
		}

		TierButtons[i] = Btn;
	}

	// Bind each tier button with a literal UFUNCTION (AddDynamic limitation: no variable-captured callbacks).
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

	UButton* GoBtn = MakeActionBtn(TEXT("Go"));
	UButton* CancelBtn = MakeActionBtn(TEXT("Cancel"));

	if (GoBtn)     { GoBtn->OnClicked.AddDynamic(this, &UMapSelectWidget::OnGoClicked); }
	if (CancelBtn) { CancelBtn->OnClicked.AddDynamic(this, &UMapSelectWidget::OnCancelClicked); }

	return true;
}

void UMapSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Reflect whatever tier the GameInstance already has stored (in case the player re-opens the menu).
	if (UDungeonGameInstance* GI = GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
	{
		CurrentTier = GI->GetSelectedTier();
	}
	RefreshTierLabels();
}

void UMapSelectWidget::Init(FName InTargetMap)
{
	TargetMap = InTargetMap;
	// Show the actual destination (one map for now; the list grows when more maps exist).
	if (MapNameLabel && !TargetMap.IsNone())
	{
		MapNameLabel->SetText(FText::FromName(TargetMap));
	}
}

// ---- Tier helpers ----

void UMapSelectWidget::SelectTier(int32 Tier)
{
	CurrentTier = FMath::Clamp(Tier, 0, 3);
	RefreshTierLabels();
}

void UMapSelectWidget::RefreshTierLabels()
{
	static const TCHAR* TierDescriptions[4] =
	{
		TEXT("Tier: 0  (Normal)"),
		TEXT("Tier: 1  (Hard)"),
		TEXT("Tier: 2  (Elite)"),
		TEXT("Tier: 3  (Nightmare)"),
	};

	if (TierLabel)
	{
		TierLabel->SetText(FText::FromString(TierDescriptions[CurrentTier]));
	}

	// Highlight the selected tier button with a tinted style; dim the others.
	for (int32 i = 0; i < 4; ++i)
	{
		if (!TierButtons[i]) { continue; }

		// Use tint colour on the button's colour-and-opacity to indicate selection.
		const FLinearColor Tint = (i == CurrentTier)
			? FLinearColor(0.20f, 0.60f, 1.00f, 1.f)   // cyan highlight
			: FLinearColor(1.f, 1.f, 1.f, 0.55f);       // dim/unselected
		TierButtons[i]->SetColorAndOpacity(Tint);
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
	// Persist the choice on the GameInstance: SelectedTier is read at boss spawn (ABossArena). SelectedMap
	// is reserved for when multiple maps exist — the travel below uses MapToTravel directly for now.
	const FName MapToTravel = TargetMap.IsNone() ? FName(TEXT("L_DungeonTest")) : TargetMap;

	UDungeonGameInstance* GI = GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	if (GI)
	{
		GI->SetSelectedTier(CurrentTier);
		GI->SetSelectedMap(MapToTravel);
	}

	// Save the player profile before leaving (mirrors the existing portal Interact save).
	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		Player->SaveNow();
	}

	// Close the menu first, then trigger the fade + travel via the player controller.
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
