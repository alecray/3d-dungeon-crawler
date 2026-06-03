#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootSlotWidget.generated.h"

class UButton;
class UTextBlock;
class ALootChest;
class UInventoryComponent;

/** One row in the loot pane: a button showing an item; clicking it takes the item into the player. */
UCLASS()
class DUNGEONCRAWLER_API ULootSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	void Setup(ALootChest* InChest, int32 InIndex, UInventoryComponent* InInventory);

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY() TObjectPtr<UButton> Button;
	UPROPERTY() TObjectPtr<UTextBlock> Label;
	TWeakObjectPtr<ALootChest> Chest;
	TWeakObjectPtr<UInventoryComponent> Inventory;
	int32 Index = INDEX_NONE;
};
