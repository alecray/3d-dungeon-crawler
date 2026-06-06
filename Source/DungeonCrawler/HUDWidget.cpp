#include "HUDWidget.h"
#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"
#include "StatsComponent.h"
#include "DungeonGameInstance.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

static constexpr float DamagePopupDuration = 0.9f; // life of the "-N" HP-loss popup
static constexpr float HitMarkerDuration = 0.18f;  // flash time of the center hit marker
static constexpr float LevelUpDuration = 1.6f;     // life of the "LEVEL UP!" banner

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

	// "-N" damage popup that flashes over the HP bar when the player is hit, then drifts off + fades.
	DamagePopup = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamagePopup"));
	DamagePopup->SetVisibility(ESlateVisibility::Collapsed);
	DamagePopup->SetJustification(ETextJustify::Center);
	DamagePopup->SetColorAndOpacity(FLinearColor(1.f, 0.3f, 0.25f)); // red
	if (FSlateFontInfo F = DamagePopup->GetFont(); true) { F.Size = 22; DamagePopup->SetFont(F); }
	if (UCanvasPanelSlot* DS = Root->AddChildToCanvas(DamagePopup))
	{
		DS->SetAnchors(FAnchors(0.f, 1.f));          // bottom-left, over the HP bar
		DS->SetAlignment(FVector2D(0.5f, 1.f));
		DS->SetPosition(FVector2D(164.f, -98.f));    // centered above the 24..304 HP bar
		DS->SetAutoSize(true);
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

	// Hit marker ("✕"), centered, flashes when a hit lands.
	HitMarker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HitMarker"));
	HitMarker->SetText(FText::FromString(TEXT("✕")));
	HitMarker->SetJustification(ETextJustify::Center);
	HitMarker->SetColorAndOpacity(FLinearColor(1.f, 0.85f, 0.25f)); // gold
	HitMarker->SetVisibility(ESlateVisibility::Collapsed);
	if (FSlateFontInfo F = HitMarker->GetFont(); true) { F.Size = 26; HitMarker->SetFont(F); }
	if (UCanvasPanelSlot* HS = Root->AddChildToCanvas(HitMarker))
	{
		HS->SetAnchors(FAnchors(0.5f, 0.5f));
		HS->SetAlignment(FVector2D(0.5f, 0.5f));
		HS->SetPosition(FVector2D(0.f, 0.f));
		HS->SetAutoSize(true);
	}

	// "LEVEL UP!" banner, centered a bit above the middle, flashes on level-up.
	LevelUpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelUpText"));
	LevelUpText->SetText(FText::FromString(TEXT("LEVEL UP!")));
	LevelUpText->SetJustification(ETextJustify::Center);
	LevelUpText->SetColorAndOpacity(FLinearColor(1.f, 0.86f, 0.3f));
	LevelUpText->SetVisibility(ESlateVisibility::Collapsed);
	if (FSlateFontInfo F = LevelUpText->GetFont(); true) { F.Size = 44; LevelUpText->SetFont(F); }
	if (UCanvasPanelSlot* LS = Root->AddChildToCanvas(LevelUpText))
	{
		LS->SetAnchors(FAnchors(0.5f, 0.5f));
		LS->SetAlignment(FVector2D(0.5f, 0.5f));
		LS->SetPosition(FVector2D(0.f, -140.f));
		LS->SetAutoSize(true);
	}

	// Version stamp, bottom-right corner.
	UTextBlock* Version = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Version"));
	Version->SetText(FText::FromString(UDungeonGameInstance::GetGameVersion()));
	Version->SetColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.75f, 0.6f));
	if (FSlateFontInfo F = Version->GetFont(); true) { F.Size = 13; Version->SetFont(F); }
	if (UCanvasPanelSlot* VS = Root->AddChildToCanvas(Version))
	{
		VS->SetAnchors(FAnchors(1.f, 1.f));        // bottom-right
		VS->SetAlignment(FVector2D(1.f, 1.f));
		VS->SetPosition(FVector2D(-14.f, -10.f));
		VS->SetAutoSize(true);
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

	if (const UHealthComponent* H = Player->GetHealthComponent())
	{
		if (HealthBar) { HealthBar->SetPercent(H->GetHealthPercent()); }

		// Trigger a "-N" popup whenever the player loses HP.
		const float Cur = H->GetHealth();
		if (LastHealth >= 0.f && Cur < LastHealth - 0.5f && DamagePopup)
		{
			const int32 Lost = FMath::RoundToInt(LastHealth - Cur);
			DamagePopup->SetText(FText::FromString(FString::Printf(TEXT("-%d"), Lost)));
			DamagePopupTime = DamagePopupDuration;
			const float Ang = FMath::FRandRange(0.f, 2.f * PI); // random drift direction
			DamagePopupDir = FVector2D(FMath::Cos(Ang), FMath::Sin(Ang));
		}
		if (Cur >= 0.f) { LastHealth = Cur; }
	}

	// Animate the active damage popup: a quick pop, drift in its random direction, fade out.
	if (DamagePopup)
	{
		if (DamagePopupTime > 0.f)
		{
			DamagePopupTime = FMath::Max(0.f, DamagePopupTime - InDeltaTime);
			const float A = 1.f - (DamagePopupTime / DamagePopupDuration); // 0 -> 1 over its life
			DamagePopup->SetVisibility(ESlateVisibility::HitTestInvisible);
			DamagePopup->SetRenderOpacity(1.f - A);
			DamagePopup->SetRenderTranslation(DamagePopupDir * (A * 55.f));
			const float Pop = (A < 0.2f) ? FMath::Lerp(1.5f, 1.f, A / 0.2f) : 1.f;
			DamagePopup->SetRenderScale(FVector2D(Pop, Pop));
		}
		else if (DamagePopup->GetVisibility() != ESlateVisibility::Collapsed)
		{
			DamagePopup->SetVisibility(ESlateVisibility::Collapsed);
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
			const int32 Lv = St->GetLevel();
			LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv %d"), Lv)));
			if (LastLevelSeen >= 0 && Lv > LastLevelSeen) { LevelUpTime = LevelUpDuration; } // celebrate
			LastLevelSeen = Lv;
		}
	}

	// Hit marker: flash when the player lands a melee hit (poll the player's last-hit timestamp).
	{
		const double HitT = Player->GetLastHitLandedTime();
		if (HitT > LastHitSeen) { HitMarkerTime = HitMarkerDuration; LastHitSeen = HitT; }
	}
	if (HitMarker)
	{
		if (HitMarkerTime > 0.f)
		{
			HitMarkerTime = FMath::Max(0.f, HitMarkerTime - InDeltaTime);
			const float T = HitMarkerTime / HitMarkerDuration; // 1 -> 0
			HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
			HitMarker->SetRenderOpacity(T);
			const float S = FMath::Lerp(1.0f, 1.6f, T); // shrinks inward as it fades
			HitMarker->SetRenderScale(FVector2D(S, S));
		}
		else if (HitMarker->GetVisibility() != ESlateVisibility::Collapsed)
		{
			HitMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// "LEVEL UP!" banner: pop in, hold, rise + fade out.
	if (LevelUpText)
	{
		if (LevelUpTime > 0.f)
		{
			LevelUpTime = FMath::Max(0.f, LevelUpTime - InDeltaTime);
			const float A = 1.f - (LevelUpTime / LevelUpDuration); // 0 -> 1 over its life
			LevelUpText->SetVisibility(ESlateVisibility::HitTestInvisible);
			LevelUpText->SetRenderOpacity(A < 0.7f ? 1.f : FMath::GetMappedRangeValueClamped(FVector2D(0.7f, 1.f), FVector2D(1.f, 0.f), A));
			LevelUpText->SetRenderTranslation(FVector2D(0.f, -40.f * A)); // drift up
			const float Pop = (A < 0.15f) ? FMath::Lerp(0.6f, 1.f, A / 0.15f) : 1.f;
			LevelUpText->SetRenderScale(FVector2D(Pop, Pop));
		}
		else if (LevelUpText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			LevelUpText->SetVisibility(ESlateVisibility::Collapsed);
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
