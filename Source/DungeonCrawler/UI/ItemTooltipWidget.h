#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltipWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * Styled hover tooltip for an item: black panel, item name in white, type+rarity line colored by rarity,
 * description + value in yellow. Built in C++ (no .uasset). Call SetItem to populate before it shows.
 */
UCLASS()
class DUNGEONCRAWLER_API UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Fill the tooltip from an item id; ExtraLine adds a dim hint at the bottom (e.g. "(click to unequip)"). */
	void SetItem(FName ItemId, const FString& ExtraLine = FString());

private:
	UPROPERTY() TObjectPtr<UBorder> Background;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> TypeText;
	UPROPERTY() TObjectPtr<UTextBlock> DescText;
	UPROPERTY() TObjectPtr<UTextBlock> ValueText;
	UPROPERTY() TObjectPtr<UTextBlock> HintText;
};
