#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UInventoryComponent;
class AFirstPersonCharacter;
class UShopWidget;

/** Per-row click handler for the shop (UButton::OnClicked carries no sender). */
UCLASS()
class UShopClickProxy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnClicked();

	bool bSell = false;   // false = buy ware, true = sell inventory slot
	FName ItemId;         // ware id (buy)
	int32 SlotIndex = -1; // inventory slot (sell)
	UPROPERTY() TWeakObjectPtr<UShopWidget> Owner;
};

/**
 * Pure-C++ shop screen: a Buy column (the NPC's wares at item value) and a Sell column (the player's
 * inventory at half value). Buying/selling adjusts gold + inventory and persists via the player.
 */
UCLASS()
class DUNGEONCRAWLER_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** The wares to offer for sale (set by the NPC before the widget is shown). */
	void SetWares(const TArray<FName>& InWares) { Wares = InWares; }

	void Buy(FName ItemId);
	void Sell(int32 SlotIndex);
	void Refresh();

private:
	AFirstPersonCharacter* GetPlayer() const;
	UInventoryComponent* GetInventory() const;
	void HandleInventoryChanged(UInventoryComponent*);

	UPROPERTY() TObjectPtr<UTextBlock> GoldText;
	UPROPERTY() TObjectPtr<UVerticalBox> BuyList;
	UPROPERTY() TObjectPtr<UVerticalBox> SellList;
	UPROPERTY() TArray<TObjectPtr<UShopClickProxy>> Proxies; // kept alive for OnClicked

	UPROPERTY() TArray<FName> Wares;

	FDelegateHandle InventoryChangedHandle;
};
