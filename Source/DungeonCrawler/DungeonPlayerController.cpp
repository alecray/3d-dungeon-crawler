#include "DungeonPlayerController.h"
#include "HUDWidget.h"
#include "HotbarWidget.h"
#include "InventoryWidget.h"
#include "MinimapWidget.h"
#include "SkillTreeWidget.h"
#include "ShopWidget.h"
#include "ShopNPC.h"
#include "PauseMenuWidget.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
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
			InventoryWidget->SetShowEquipment(true);                  // paperdoll beside the grid
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

void ADungeonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		// Esc must work while the game is paused, so bind it as a raw key with bExecuteWhenPaused.
		FInputKeyBinding& Binding = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
			&ADungeonPlayerController::TogglePauseMenu);
		Binding.bExecuteWhenPaused = true;
	}
}

void ADungeonPlayerController::TogglePauseMenu()
{
	if (IsPauseMenuOpen())
	{
		ClosePauseMenu();
		return;
	}
	PauseWidget = CreateWidget<UUserWidget>(this, UPauseMenuWidget::StaticClass());
	if (PauseWidget)
	{
		PauseWidget->AddToViewport(50); // above all other UI
	}
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());
}

void ADungeonPlayerController::ClosePauseMenu()
{
	if (PauseWidget && PauseWidget->IsInViewport())
	{
		PauseWidget->RemoveFromParent();
	}
	PauseWidget = nullptr;
	UGameplayStatics::SetGamePaused(this, false);
	UpdateInputMode(); // restore cursor/input based on any remaining open UI
}

bool ADungeonPlayerController::IsPauseMenuOpen() const
{
	return PauseWidget && PauseWidget->IsInViewport();
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

void ADungeonPlayerController::OpenShop(AShopNPC* NPC)
{
	if (!NPC)
	{
		return;
	}
	if (UShopWidget* ExistingShop = Cast<UShopWidget>(ShopWidget))
	{
		// already open: refresh wares
		ExistingShop->SetWares(NPC->GetWares());
		ExistingShop->Refresh();
	}
	else if (UShopWidget* NewShop = CreateWidget<UShopWidget>(this, UShopWidget::StaticClass()))
	{
		NewShop->SetWares(NPC->GetWares());
		ShopWidget = NewShop;
		NewShop->AddToViewport(10);
	}
	UpdateInputMode();
}

void ADungeonPlayerController::CloseShop()
{
	if (ShopWidget && ShopWidget->IsInViewport())
	{
		ShopWidget->RemoveFromParent();
	}
	ShopWidget = nullptr;
	UpdateInputMode();
}

bool ADungeonPlayerController::IsShopOpen() const
{
	return ShopWidget && ShopWidget->IsInViewport();
}

void ADungeonPlayerController::OpenLootMenu(ALootChest* Chest)
{
	if (!Chest || !InventoryWidgetClass)
	{
		return;
	}
	Chest->Open(); // roll loot (first time) + play the open animation
	OpenChest = Chest;

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
		InventoryWidget->SetShowEquipment(false); // loot view: grid only, no paperdoll
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
	if (OpenChest.IsValid())
	{
		OpenChest->CloseLid(); // play the chest's close animation
		OpenChest = nullptr;
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
		(SkillWidget && SkillWidget->IsInViewport()) ||
		(ShopWidget && ShopWidget->IsInViewport());

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
