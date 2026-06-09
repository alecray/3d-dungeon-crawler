#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapSelectWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;

/**
 * Pure-C++ map + tier select panel shown when the player interacts with an active town portal.
 *
 * Layout:
 *   Title
 *   Map list  (one entry per available destination; currently just the portal's target map)
 *   Tier selector  (0–3 buttons; 0 = default, 1–3 = scaling difficulty)
 *   [ Go ]  [ Cancel ]
 *
 * Call Init() with the portal's target map name before adding to the viewport.
 * Go  -> writes selection to GameInstance, saves profile, calls FadeToBlackAndTravel.
 * Cancel -> removes the widget and restores game input.
 */
UCLASS()
class DUNGEONCRAWLER_API UMapSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;

	/**
	 * Set the available destination(s) before showing the widget.  The portal's target map name is
	 * required; the widget stores it as the sole selectable map entry for now.
	 */
	void Init(FName InTargetMap);

private:
	// --- button handlers ---
	UFUNCTION() void OnGoClicked();
	UFUNCTION() void OnCancelClicked();

	UFUNCTION() void OnTier0Clicked();
	UFUNCTION() void OnTier1Clicked();
	UFUNCTION() void OnTier2Clicked();
	UFUNCTION() void OnTier3Clicked();

	void SelectTier(int32 Tier);
	void RefreshTierLabels();

	FName TargetMap;   // the single available destination (set by Init)
	int32 CurrentTier = 0;

	// Widget refs (kept alive as UPROPERTYs so GC doesn't collect them under the widget tree).
	UPROPERTY() TObjectPtr<UTextBlock> MapNameLabel;         // destination name (refreshed by Init)
	UPROPERTY() TObjectPtr<UTextBlock> TierLabel;            // "Tier: X" display
	UPROPERTY() TObjectPtr<UButton>    TierButtons[4];       // 0..3
	UPROPERTY() TObjectPtr<UTextBlock> TierBtnTexts[4];      // text inside each tier button (for lock labels)
};
