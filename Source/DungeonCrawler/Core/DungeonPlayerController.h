#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DungeonPlayerController.generated.h"

class UHUDWidget;
class UUserWidget;
class UMinimapWidget;
class UInventoryWidget;
class UInventoryComponent;
class ALootChest;
class AShopNPC;
enum class ECharacterClass : uint8; // defined in CharacterClass.h

/**
 * Player controller that owns the on-screen UI: HUD, inventory grid, collection log, and the chest
 * loot view (the chest's grid shown beside the player's so items can be dragged across).
 */
UCLASS()
class DUNGEONCRAWLER_API ADungeonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADungeonPlayerController();

	void ToggleInventory();      // I
	void ToggleCollectionLog();  // C
	void ToggleSkillTree();      // K

	/** Esc: pause + show the pause menu, or resume + hide it. */
	void TogglePauseMenu();
	void ClosePauseMenu();
	bool IsPauseMenuOpen() const;

	/** Open the chest + player inventory grids side by side; loot by dragging across. */
	void OpenLootMenu(ALootChest* Chest);
	void CloseLootMenu();
	bool IsLootMenuOpen() const;

	/** Open/close the shop UI for an NPC's wares. */
	void OpenShop(AShopNPC* NPC);
	void CloseShop();
	bool IsShopOpen() const;

	/** Start-menu Start button: dismiss the menu, hand back control, and fade in from black. */
	void StartGameFromMenu();

	/** Class-select start: apply the chosen archetype's loadout to a fresh character, then start. On a
	    returning profile (already has saved progress) the class is ignored and it just continues. */
	void StartGameAsClass(ECharacterClass Class);

	/** Fade the screen to black over Duration (s), then open Map — the standard scene transition. */
	void FadeToBlackAndTravel(FName Map, float Duration = 0.45f);

	// Dev menu helpers (invoked from the pause menu's Dev panel).
	void DevRevealMap();
	void DevTeleportHome();
	void DevTeleportToBoss();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CollectionWidgetClass;

private:
	void UpdateInputMode();
	UInventoryComponent* GetPlayerInventory() const;

	/** Shows the start menu over the boot map and freezes the player until Start is pressed. */
	void ShowMainMenu();
	/** Fades the camera in from black (e.g. on arriving in a level). */
	void FadeFromBlack(float Duration = 0.5f);
	/** Timer callback: performs the deferred OpenLevel after the fade-to-black. */
	void DoPendingTravel();

	UPROPERTY() TObjectPtr<UUserWidget> MainMenuWidget;
	FName PendingTravelMap;
	FTimerHandle TravelTimer;

	UPROPERTY() TObjectPtr<UMinimapWidget> MinimapWidget;
	UPROPERTY() TObjectPtr<UUserWidget> HUDWidget;
	UPROPERTY() TObjectPtr<UInventoryWidget> InventoryWidget;
	UPROPERTY() TObjectPtr<UUserWidget> CollectionWidget;
	UPROPERTY() TObjectPtr<UInventoryWidget> ChestPane;
	UPROPERTY() TObjectPtr<UUserWidget> SkillWidget;
	UPROPERTY() TObjectPtr<UUserWidget> ShopWidget;
	UPROPERTY() TObjectPtr<UUserWidget> PauseWidget;

	/** The chest whose loot pane is open, so its lid can be closed when the menu closes. */
	TWeakObjectPtr<ALootChest> OpenChest;

protected:
	virtual void SetupInputComponent() override;
};
