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
};
