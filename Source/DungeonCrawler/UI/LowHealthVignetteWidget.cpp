#include "LowHealthVignetteWidget.h"
#include "HealthComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

bool ULowHealthVignetteWidget::Initialize()
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

	Overlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Overlay"));
	Overlay->SetColorAndOpacity(FLinearColor(0.6f, 0.f, 0.f, 0.f)); // red, fully transparent at rest
	Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);      // never eat clicks
	if (UCanvasPanelSlot* OS = Root->AddChildToCanvas(Overlay))
	{
		OS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		OS->SetOffsets(FMargin(0.f));
	}

	return true;
}

void ULowHealthVignetteWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (!Overlay)
	{
		return;
	}

	float Pct = 1.f;
	float Cur = -1.f;
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>())
		{
			Pct = Health->GetHealthPercent();
			Cur = Health->GetHealth();
		}
	}

	// Red flash whenever the player actually loses HP (took a hit). Compare absolute HP, not percent —
	// equipping gear that raises Max Health lowers the percent with no damage, which used to false-flash.
	if (LastHealth >= 0.f && Cur >= 0.f && Cur < LastHealth - 0.01f)
	{
		FlashTime = 0.22f;
	}
	if (Cur >= 0.f) { LastHealth = Cur; }
	FlashTime = FMath::Max(0.f, FlashTime - DeltaTime);
	const float FlashAlpha = (FlashTime / 0.22f) * 0.4f;

	// Low-health pulse ramps from 0 at the threshold to 1 at empty; pulse faster the lower it gets.
	const float Intensity = FMath::Clamp((Threshold - Pct) / Threshold, 0.f, 1.f);
	Pulse += DeltaTime * (4.f + 6.f * Intensity);
	const float PulseMul = 0.55f + 0.45f * FMath::Sin(Pulse);
	const float LowHpAlpha = Intensity * PulseMul * MaxOpacity;

	Overlay->SetColorAndOpacity(FLinearColor(0.6f, 0.f, 0.f, FMath::Max(LowHpAlpha, FlashAlpha)));
}
