#include "DungeonPlayerController.h"
#include "HUDWidget.h"
#include "Blueprint/UserWidget.h"

ADungeonPlayerController::ADungeonPlayerController()
{
	HUDWidgetClass = UHUDWidget::StaticClass();
}

void ADungeonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}
