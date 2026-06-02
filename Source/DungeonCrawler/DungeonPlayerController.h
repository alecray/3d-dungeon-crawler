#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DungeonPlayerController.generated.h"

class UHUDWidget;
class UUserWidget;

/**
 * Player controller that owns the on-screen UI. Phase 1 creates the HUD; later phases add the
 * inventory/skills/shop widgets and their toggle inputs.
 */
UCLASS()
class DUNGEONCRAWLER_API ADungeonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADungeonPlayerController();

protected:
	virtual void BeginPlay() override;

	/** HUD widget class to spawn (defaults to UHUDWidget). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;
};
