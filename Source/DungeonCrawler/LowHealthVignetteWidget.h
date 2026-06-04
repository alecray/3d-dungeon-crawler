#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LowHealthVignetteWidget.generated.h"

class UImage;

/**
 * A full-screen red danger overlay that fades in and pulses as the player's health drops below a
 * threshold (a stand-in for a proper edge vignette + heartbeat until art/audio land). Polls the local
 * player's health each frame, so it needs no wiring beyond being added to the viewport.
 */
UCLASS()
class DUNGEONCRAWLER_API ULowHealthVignetteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	UPROPERTY() TObjectPtr<UImage> Overlay;

private:
	float Pulse = 0.f;

	/** Health fraction at/below which the overlay starts showing. */
	static constexpr float Threshold = 0.35f;
	/** Peak opacity at 0 HP. */
	static constexpr float MaxOpacity = 0.34f;
};
