#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UHealthComponent;
class ABossMonster;

/**
 * A dedicated boss health bar pinned to the top of the screen during a boss fight (separate from the
 * player HUD). Code-built (graybox): a name label over a red progress bar that tracks the boss's
 * UHealthComponent each frame. Created/removed by ABossArena for the encounter.
 */
UCLASS()
class DUNGEONCRAWLER_API UBossHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Binds the bar to a boss and labels it. Call right after creating the widget. */
	void SetBoss(ABossMonster* Boss);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	UPROPERTY() TObjectPtr<UProgressBar> Bar;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	/** Unicode star row: "★ ★" remaining / "☆ ☆" used. Hidden when MaxPhases <= 1. */
	UPROPERTY() TObjectPtr<UTextBlock> PhaseText;

private:
	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<ABossMonster>     BossPtr;

	/** Rebuild the star string and update PhaseText visibility. */
	void RefreshPhaseStars();
};
