#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootWidget.generated.h"

class UVerticalBox;
class UButton;
class ALootChest;
class UInventoryComponent;

/** Loot pane (pure C++): shows a chest's contents as click-to-take rows, plus Take All / Close. */
UCLASS()
class DUNGEONCRAWLER_API ULootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bind this pane to a chest (call before/after adding to viewport). */
	void SetChest(ALootChest* InChest);

private:
	void Rebuild(ALootChest* Changed = nullptr);

	UFUNCTION() void OnTakeAllClicked();
	UFUNCTION() void OnCloseClicked();

	UPROPERTY() TObjectPtr<UVerticalBox> List;
	UPROPERTY() TObjectPtr<UButton> TakeAllButton;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	TWeakObjectPtr<ALootChest> Chest;
	TWeakObjectPtr<UInventoryComponent> Inventory;
};
