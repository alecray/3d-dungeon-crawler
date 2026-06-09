#include "BossHealthBarWidget.h"
#include "BossMonster.h"
#include "HealthComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

bool UBossHealthBarWidget::Initialize()
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

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	if (UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(Box))
	{
		// Pin to top-center.
		BoxSlot->SetAnchors(FAnchors(0.5f, 0.f));
		BoxSlot->SetAlignment(FVector2D(0.5f, 0.f));
		BoxSlot->SetPosition(FVector2D(0.f, 40.f));
		BoxSlot->SetSize(FVector2D(720.f, 56.f));
	}

	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	NameText->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo Font = NameText->GetFont(); true) { Font.Size = 22; NameText->SetFont(Font); }
	if (UVerticalBoxSlot* NS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(NameText)))
	{
		NS->SetHorizontalAlignment(HAlign_Center);
		NS->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar"));
	Bar->SetPercent(1.f);
	Bar->SetFillColorAndOpacity(FLinearColor(0.8f, 0.07f, 0.07f, 1.f)); // boss red
	if (UVerticalBoxSlot* BarSlot = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Bar)))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	// Phase star row — Unicode filled/empty stars, asset-free.  Hidden until we know MaxPhases > 1.
	PhaseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PhaseText"));
	PhaseText->SetJustification(ETextJustify::Center);
	PhaseText->SetColorAndOpacity(FLinearColor(1.f, 0.85f, 0.25f)); // gold
	if (FSlateFontInfo F = PhaseText->GetFont(); true) { F.Size = 18; PhaseText->SetFont(F); }
	PhaseText->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* PS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(PhaseText)))
	{
		PS->SetHorizontalAlignment(HAlign_Center);
		PS->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	return true;
}

void UBossHealthBarWidget::SetBoss(ABossMonster* Boss)
{
	if (!Boss)
	{
		return;
	}
	BossPtr = Boss;
	Health  = Boss->FindComponentByClass<UHealthComponent>();
	if (NameText)
	{
		// Turn the boss id ("HermitCrab") into a readable label ("HERMIT CRAB").
		const FString Id = Boss->GetBossId().ToString();
		FString Spaced;
		for (int32 i = 0; i < Id.Len(); ++i)
		{
			if (i > 0 && FChar::IsUpper(Id[i]) && !FChar::IsUpper(Id[i - 1])) { Spaced.AppendChar(' '); }
			Spaced.AppendChar(Id[i]);
		}
		NameText->SetText(FText::FromString(Spaced.ToUpper()));
	}
	// Populate phase stars immediately so the row is correct before the first tick.
	RefreshPhaseStars();
}

void UBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (Bar && Health.IsValid())
	{
		Bar->SetPercent(Health->GetHealthPercent());
	}

	RefreshPhaseStars();
}

void UBossHealthBarWidget::RefreshPhaseStars()
{
	if (!PhaseText || !BossPtr.IsValid())
	{
		return;
	}

	const int32 Max       = BossPtr->GetMaxPhases();
	const int32 Remaining = BossPtr->GetPhasesRemaining();

	// Nothing meaningful to show for single-phase bosses.
	if (Max <= 1)
	{
		PhaseText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Build the star string: filled stars for lives left, dim stars for lives spent.
	// e.g. Max=3, Remaining=2 → "★ ★ ☆"
	FString Stars;
	Stars.Reserve(Max * 2);
	for (int32 i = 0; i < Max; ++i)
	{
		if (i > 0) { Stars.AppendChar(TEXT(' ')); }
		Stars.AppendChar(i < Remaining ? TEXT('★') : TEXT('☆'));
	}

	PhaseText->SetText(FText::FromString(Stars));
	// Dim the empty stars by tinting the whole block; the filled gold stars pop against it.
	PhaseText->SetColorAndOpacity(FLinearColor(1.f, 0.85f, 0.25f));
	PhaseText->SetVisibility(ESlateVisibility::HitTestInvisible);
}
