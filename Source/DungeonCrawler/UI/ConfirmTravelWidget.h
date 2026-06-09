#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmTravelWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Simple "Leave Dungeon?" confirmation dialog shown when the player interacts with a return portal
 * inside the dungeon. Two buttons: Leave (fades to black and travels to town) and Stay (closes).
 */
UCLASS()
class DUNGEONCRAWLER_API UConfirmTravelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Set the destination map before adding to viewport. */
	void Init(FName InTargetMap);

private:
	UFUNCTION() void OnLeaveClicked();
	UFUNCTION() void OnStayClicked();

	FName TargetMap;
};
