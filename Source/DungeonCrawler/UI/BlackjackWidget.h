#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlackjackWidget.generated.h"

class UTextBlock;
class UButton;
class UHorizontalBox;
class ABlackjackTable;
class APlayerController;

/**
 * Slim on-screen control bar for the 3D blackjack table: gold/bet readout + status line and the
 * Bet± / Deal / Hit / Stand / Leave buttons. Holds no game state — it just drives ABlackjackTable
 * (which owns the rules + the 3D cards) and reflects its phase.
 */
UCLASS()
class DUNGEONCRAWLER_API UBlackjackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Shows the bar for this table/controller and captures the cursor. */
	void OpenFor(ABlackjackTable* Table, APlayerController* PC);

	/** Re-reads the table's phase/gold/bet/status and updates labels + button enable states. */
	void RefreshFromTable();

	/** Removes the bar and hands input back to the game. */
	void CloseBar();

protected:
	UFUNCTION() void OnBetDown();
	UFUNCTION() void OnBetUp();
	UFUNCTION() void OnDeal();
	UFUNCTION() void OnHit();
	UFUNCTION() void OnStand();
	UFUNCTION() void OnLeave();

private:
	UButton* MakeButton(UHorizontalBox* Row, const TCHAR* Label, void (UBlackjackWidget::*Handler)());

	UPROPERTY() TObjectPtr<UTextBlock> GoldText;
	UPROPERTY() TObjectPtr<UTextBlock> BetText;
	UPROPERTY() TObjectPtr<UTextBlock> StatusText;
	UPROPERTY() TObjectPtr<UButton> BetDownBtn;
	UPROPERTY() TObjectPtr<UButton> BetUpBtn;
	UPROPERTY() TObjectPtr<UButton> DealBtn;
	UPROPERTY() TObjectPtr<UButton> HitBtn;
	UPROPERTY() TObjectPtr<UButton> StandBtn;

	UPROPERTY() TWeakObjectPtr<ABlackjackTable> Table;
};
