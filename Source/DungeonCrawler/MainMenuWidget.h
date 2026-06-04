#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

/**
 * The start screen. A code-built (graybox) full-screen opaque menu shown at launch: a title and
 * Start / Quit buttons. Start hands off to the player controller to begin play (fade in from black);
 * Quit exits. Swap the background/buttons for art later.
 */
UCLASS()
class DUNGEONCRAWLER_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnQuitClicked();
};
