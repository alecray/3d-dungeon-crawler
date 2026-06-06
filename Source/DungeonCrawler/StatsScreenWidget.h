#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatsScreenWidget.generated.h"

class UVerticalBox;

/**
 * Full-screen "Stats" page listing lifetime play statistics from the saved profile (FPlayerStats).
 * Code-built (graybox); opened from the title screen and the pause menu, closed with Back (removes itself
 * so the menu behind it returns). Reads the stats fresh on each open.
 */
UCLASS()
class DUNGEONCRAWLER_API UStatsScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnBackClicked();

private:
	/** Fills the rows from the game instance's saved stats. */
	void Populate();

	UPROPERTY() TObjectPtr<UVerticalBox> RowsBox;
};
