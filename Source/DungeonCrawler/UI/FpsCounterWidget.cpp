#include "FpsCounterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Misc/App.h"

bool UFpsCounterWidget::Initialize()
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

	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
	Label->SetText(FText::FromString(TEXT("-- FPS")));
	Label->SetJustification(ETextJustify::Right);
	if (FSlateFontInfo Font = Label->GetFont(); true) { Font.Size = 16; Label->SetFont(Font); }
	if (UCanvasPanelSlot* LS = Root->AddChildToCanvas(Label))
	{
		// Top-right, just below the 220px minimap (which sits at y=16, ~232px tall).
		LS->SetAnchors(FAnchors(1.f, 0.f));
		LS->SetAlignment(FVector2D(1.f, 0.f));
		LS->SetPosition(FVector2D(-16.f, 258.f));
		LS->SetAutoSize(true);
	}

	return true;
}

void UFpsCounterWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (!Label)
	{
		return;
	}

	// Smooth the real frame time (FApp delta is unaffected by hit-stop time dilation).
	const float RealDelta = FMath::Max(FApp::GetDeltaTime(), KINDA_SMALL_NUMBER);
	SmoothedDelta = FMath::FInterpTo(SmoothedDelta, RealDelta, DeltaTime, 5.f);

	const int32 Fps = FMath::RoundToInt(1.f / SmoothedDelta);
	Label->SetText(FText::FromString(FString::Printf(TEXT("%d FPS"), Fps)));

	FLinearColor Color = FLinearColor(0.3f, 1.0f, 0.4f);       // green
	if (Fps < 30)      { Color = FLinearColor(1.0f, 0.35f, 0.3f); } // red
	else if (Fps < 60) { Color = FLinearColor(1.0f, 0.85f, 0.3f); } // yellow
	Label->SetColorAndOpacity(FSlateColor(Color));
}
