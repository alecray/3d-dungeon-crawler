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

	/** Show/hide the inventory panel (bound to I). */
	void ToggleInventory();

	/** Show/hide the collection log (bound to C). */
	void ToggleCollectionLog();

protected:
	virtual void BeginPlay() override;

	/** HUD widget class to spawn (defaults to UHUDWidget). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CollectionWidgetClass;

private:
	/** Toggles a panel widget in/out of the viewport, (re)creating it as needed. */
	void TogglePanel(TObjectPtr<UUserWidget>& Widget, TSubclassOf<UUserWidget> WidgetClass);

	/** Sets mouse cursor + input mode based on whether any panel is open. */
	void UpdateInputMode();

	UPROPERTY() TObjectPtr<UUserWidget> HUDWidget;
	UPROPERTY() TObjectPtr<UUserWidget> InventoryWidget;
	UPROPERTY() TObjectPtr<UUserWidget> CollectionWidget;
};
