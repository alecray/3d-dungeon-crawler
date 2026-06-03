#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UUniformGridPanel;
class UInventoryComponent;
class UInventorySlotWidget;

/** Diablo-style inventory panel (pure C++): a centered grid of draggable slot widgets. */
UCLASS()
class DUNGEONCRAWLER_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildSlots();
	void RefreshAll(UInventoryComponent* Changed = nullptr);

	UPROPERTY() TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY() TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;
	TWeakObjectPtr<UInventoryComponent> Inventory;

	static constexpr int32 Columns = 6;
};
