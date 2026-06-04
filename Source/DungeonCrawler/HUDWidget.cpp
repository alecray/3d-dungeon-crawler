#include "HUDWidget.h"
#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"
#include "StatsComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

bool UHUDWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		return true; // already built
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Three stacked bars in the bottom-left (alignment pins to bottom-left, Y offsets go up).
	HealthBar = MakeBar(FLinearColor(0.85f, 0.15f, 0.15f), -76.f);
	ManaBar = MakeBar(FLinearColor(0.2f, 0.4f, 0.95f), -52.f);
	StaminaBar = MakeBar(FLinearColor(0.3f, 0.8f, 0.3f), -28.f);

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
	LevelText->SetText(FText::FromString(TEXT("Lv 1")));
	if (UCanvasPanelSlot* TS = Root->AddChildToCanvas(LevelText))
	{
		TS->SetAnchors(FAnchors(0.f, 1.f));
		TS->SetAlignment(FVector2D(0.f, 1.f));
		TS->SetPosition(FVector2D(24.f, -100.f));
		TS->SetAutoSize(true);
	}

	// Interaction prompt, centered just below the crosshair; hidden until looking at something usable.
	InteractPrompt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InteractPrompt"));
	InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);
	InteractPrompt->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(InteractPrompt))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.f));
		PS->SetPosition(FVector2D(0.f, 40.f));
		PS->SetAutoSize(true);
	}

	return true;
}

UProgressBar* UHUDWidget::MakeBar(const FLinearColor& Color, float YOffset)
{
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree ? WidgetTree->RootWidget : nullptr);
	if (!Root)
	{
		return nullptr;
	}

	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
	Bar->SetFillColorAndOpacity(Color);
	Bar->SetPercent(1.f);

	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bar))
	{
		S->SetAnchors(FAnchors(0.f, 1.f));      // bottom-left
		S->SetAlignment(FVector2D(0.f, 1.f));
		S->SetPosition(FVector2D(24.f, YOffset));
		S->SetSize(FVector2D(280.f, 18.f));
	}
	return Bar;
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn());
	if (!Player)
	{
		return;
	}

	if (HealthBar)
	{
		if (const UHealthComponent* H = Player->GetHealthComponent())
		{
			HealthBar->SetPercent(H->GetHealthPercent());
		}
	}
	// A denied action flashes the matching bar toward red, fading back to its normal color.
	static const FLinearColor DenyColor(1.f, 0.18f, 0.12f);
	if (ManaBar)
	{
		if (const UResourceComponent* M = Player->GetManaComponent())
		{
			ManaBar->SetPercent(M->GetPercent());
		}
		ManaBar->SetFillColorAndOpacity(FMath::Lerp(FLinearColor(0.2f, 0.4f, 0.95f), DenyColor, Player->GetManaDenyFlash()));
	}
	if (StaminaBar)
	{
		if (const UResourceComponent* S = Player->GetStaminaComponent())
		{
			StaminaBar->SetPercent(S->GetPercent());
		}
		StaminaBar->SetFillColorAndOpacity(FMath::Lerp(FLinearColor(0.3f, 0.8f, 0.3f), DenyColor, Player->GetStaminaDenyFlash()));
	}
	if (LevelText)
	{
		if (const UStatsComponent* St = Player->GetStatsComponent())
		{
			LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv %d"), St->GetLevel())));
		}
	}
	if (InteractPrompt)
	{
		const FString Verb = Player->GetInteractionPrompt();
		if (Verb.IsEmpty())
		{
			InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			InteractPrompt->SetText(FText::FromString(FString::Printf(TEXT("[E] %s"), *Verb)));
			InteractPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}
