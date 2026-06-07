#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarWidget.generated.h"

class UHorizontalBox;
class UHotbarComponent;
class UHotbarSlotWidget;

/** Bottom-center action bar (pure C++): a row of 8 hotbar slot widgets bound to the player's hotbar. */
UCLASS()
class DUNGEONCRAWLER_API UHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildSlots();
	void RefreshAll(UHotbarComponent* Changed = nullptr);

	UPROPERTY() TObjectPtr<UHorizontalBox> Row;
	UPROPERTY() TArray<TObjectPtr<UHotbarSlotWidget>> SlotWidgets;
	TWeakObjectPtr<UHotbarComponent> Hotbar;
};
