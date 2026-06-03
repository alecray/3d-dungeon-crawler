#include "DungeonPlayerController.h"
#include "HUDWidget.h"
#include "HotbarWidget.h"
#include "InventoryWidget.h"
#include "MinimapWidget.h"
#include "SkillTreeWidget.h"
#include "CollectionLogWidget.h"
#include "LootChest.h"
#include "FirstPersonCharacter.h"
#include "Blueprint/UserWidget.h"

ADungeonPlayerController::ADungeonPlayerController()
{
	HUDWidgetClass = UHUDWidget::StaticClass();
	InventoryWidgetClass = UInventoryWidget::StaticClass();
	CollectionWidgetClass = UCollectionLogWidget::StaticClass();
}

void ADungeonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		if (HUDWidgetClass)
		{
			HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
			if (HUDWidget) { HUDWidget->AddToViewport(0); }
		}
		// Always-on action bar.
		if (UHotbarWidget* Bar = CreateWidget<UHotbarWidget>(this, UHotbarWidget::StaticClass()))
		{
			Bar->AddToViewport(1);
		}
		// Always-on corner minimap.
		if (UMinimapWidget* Minimap = CreateWidget<UMinimapWidget>(this, UMinimapWidget::StaticClass()))
		{
			Minimap->AddToViewport(1);
		}
	}
}

UInventoryComponent* ADungeonPlayerController::GetPlayerInventory() const
{
	if (AFirstPersonCharacter* FPChar = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		return FPChar->GetInventoryComponent();
	}
	return nullptr;
}

void ADungeonPlayerController::ToggleInventory()
{
	if (!InventoryWidgetClass)
	{
		return;
	}
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
	}
	else
	{
		if (!InventoryWidget)
		{
			InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
		}
		if (InventoryWidget)
		{
			InventoryWidget->SetInventory(GetPlayerInventory(), TEXT("Inventory"));
			InventoryWidget->SetPanelPosition(FVector2D::ZeroVector); // centered
			InventoryWidget->AddToViewport(10);
		}
	}
	UpdateInputMode();
}

void ADungeonPlayerController::ToggleCollectionLog()
{
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

void ADungeonPlayerController::ToggleSkillTree()
{
	if (SkillWidget && SkillWidget->IsInViewport())
	{
		SkillWidget->RemoveFromParent();
		SkillWidget = nullptr;
	}
	else
	{
		SkillWidget = CreateWidget<UUserWidget>(this, USkillTreeWidget::StaticClass());
		if (SkillWidget)
		{
			SkillWidget->AddToViewport(10);
		}
	}
	UpdateInputMode();
}

void ADungeonPlayerController::OpenLootMenu(ALootChest* Chest)
{
	if (!Chest || !InventoryWidgetClass)
	{
		return;
	}
	Chest->Open(); // roll loot + flip lid (no-op if already opened)

	// Chest grid on the left.
	if (!ChestPane)
	{
		ChestPane = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
	}
	if (ChestPane)
	{
		ChestPane->SetInventory(Chest->GetInventory(), TEXT("Chest"));
		ChestPane->SetPanelPosition(FVector2D(-360.f, 0.f));
		if (!ChestPane->IsInViewport())
		{
			ChestPane->AddToViewport(10);
		}
	}

	// Player inventory on the right (so you can drag items across).
	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
	}
	if (InventoryWidget)
	{
		InventoryWidget->SetInventory(GetPlayerInventory(), TEXT("Inventory"));
		InventoryWidget->SetPanelPosition(FVector2D(360.f, 0.f));
		if (!InventoryWidget->IsInViewport())
		{
			InventoryWidget->AddToViewport(10);
		}
	}

	UpdateInputMode();
}

void ADungeonPlayerController::CloseLootMenu()
{
	if (ChestPane && ChestPane->IsInViewport())
	{
		ChestPane->RemoveFromParent();
	}
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent(); // also closes the player grid that was opened for looting
	}
	UpdateInputMode();
}

bool ADungeonPlayerController::IsLootMenuOpen() const
{
	return ChestPane && ChestPane->IsInViewport();
}

void ADungeonPlayerController::UpdateInputMode()
{
	const bool bUIOpen =
		(InventoryWidget && InventoryWidget->IsInViewport()) ||
		(CollectionWidget && CollectionWidget->IsInViewport()) ||
		(ChestPane && ChestPane->IsInViewport()) ||
		(SkillWidget && SkillWidget->IsInViewport());

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
