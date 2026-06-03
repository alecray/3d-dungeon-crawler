#include "DungeonPlayerController.h"
#include "HUDWidget.h"
#include "InventoryWidget.h"
#include "CollectionLogWidget.h"
#include "LootWidget.h"
#include "LootChest.h"
#include "Blueprint/UserWidget.h"

ADungeonPlayerController::ADungeonPlayerController()
{
	HUDWidgetClass = UHUDWidget::StaticClass();
	InventoryWidgetClass = UInventoryWidget::StaticClass();
	CollectionWidgetClass = UCollectionLogWidget::StaticClass();
	LootWidgetClass = ULootWidget::StaticClass();
}

void ADungeonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
	}
}

void ADungeonPlayerController::ToggleInventory()
{
	TogglePanel(InventoryWidget, InventoryWidgetClass);
}

void ADungeonPlayerController::ToggleCollectionLog()
{
	// The collection log rebuilds its contents each time it's shown (recreate for a fresh list).
	if (CollectionWidget && CollectionWidget->IsInViewport())
	{
		CollectionWidget->RemoveFromParent();
		CollectionWidget = nullptr;
	}
	else
	{
		CollectionWidget = CreateWidget<UUserWidget>(this, CollectionWidgetClass);
		if (CollectionWidget)
		{
			CollectionWidget->AddToViewport(10);
		}
	}
	UpdateInputMode();
}

void ADungeonPlayerController::TogglePanel(TObjectPtr<UUserWidget>& Widget, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass)
	{
		return;
	}
	if (Widget && Widget->IsInViewport())
	{
		Widget->RemoveFromParent();
	}
	else
	{
		if (!Widget)
		{
			Widget = CreateWidget<UUserWidget>(this, WidgetClass);
		}
		if (Widget)
		{
			Widget->AddToViewport(10);
		}
	}
	UpdateInputMode();
}

void ADungeonPlayerController::OpenLootMenu(ALootChest* Chest)
{
	if (!Chest || !LootWidgetClass)
	{
		return;
	}
	Chest->Open(); // roll loot + flip lid (no-op if already opened)

	if (!LootWidget)
	{
		LootWidget = CreateWidget<ULootWidget>(this, LootWidgetClass);
	}
	if (LootWidget)
	{
		LootWidget->SetChest(Chest);
		if (!LootWidget->IsInViewport())
		{
			LootWidget->AddToViewport(10);
		}
	}
	UpdateInputMode();
}

void ADungeonPlayerController::CloseLootMenu()
{
	if (LootWidget && LootWidget->IsInViewport())
	{
		LootWidget->RemoveFromParent();
	}
	UpdateInputMode();
}

bool ADungeonPlayerController::IsLootMenuOpen() const
{
	return LootWidget && LootWidget->IsInViewport();
}

void ADungeonPlayerController::UpdateInputMode()
{
	const bool bUIOpen =
		(InventoryWidget && InventoryWidget->IsInViewport()) ||
		(CollectionWidget && CollectionWidget->IsInViewport()) ||
		(LootWidget && LootWidget->IsInViewport());

	bShowMouseCursor = bUIOpen;
	if (bUIOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}
