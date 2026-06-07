#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FpsCounterWidget.generated.h"

class UTextBlock;

/**
 * A lightweight FPS read-out pinned to the top-right corner (just below the minimap). Shows a smoothed
 * frames-per-second value each frame, color-coded green/yellow/red. Code-built, no asset.
 */
UCLASS()
class DUNGEONCRAWLER_API UFpsCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	UPROPERTY() TObjectPtr<UTextBlock> Label;

private:
	float SmoothedDelta = 1.f / 60.f;
};
