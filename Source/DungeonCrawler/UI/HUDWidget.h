#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Player HUD built entirely in C++ (no .uasset widget): three stacked bars (health / mana / stamina)
 * in the bottom-left, polled each frame from the owning player's components.
 */
UCLASS()
class DUNGEONCRAWLER_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UProgressBar* MakeBar(const FLinearColor& Color, float YOffset);

	UPROPERTY() TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY() TObjectPtr<UProgressBar> ManaBar;
	UPROPERTY() TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY() TObjectPtr<UTextBlock> LevelText;
	UPROPERTY() TObjectPtr<UTextBlock> InteractPrompt; // "[E] Open" when looking at an interactable
	UPROPERTY() TObjectPtr<UTextBlock> FishingStatus;  // fishing status line ("It's biting — REEL!") shown while fishing

	UPROPERTY() TObjectPtr<UTextBlock> DamagePopup;    // "-N" that pops over the HP bar when you take damage

	UPROPERTY() TObjectPtr<UTextBlock> HitMarker;      // center "✕" that flashes when you land a hit
	UPROPERTY() TObjectPtr<UTextBlock> LevelUpText;    // "LEVEL UP!" banner that flashes on level-up

	float LastHealth = -1.f;       // for detecting HP loss (-1 = not sampled yet)
	float DamagePopupTime = 0.f;   // remaining animation time on the popup
	FVector2D DamagePopupDir = FVector2D(0.f, -1.f); // random drift direction for the current popup

	float HitMarkerTime = 0.f;     // remaining flash time on the hit marker
	double LastHitSeen = 0.0;      // last player hit time the HUD has reacted to (player default -1 = none)
	float LevelUpTime = 0.f;       // remaining flash time on the level-up banner
	int32 LastLevelSeen = -1;      // last level the HUD has reacted to (-1 = not sampled)
};
