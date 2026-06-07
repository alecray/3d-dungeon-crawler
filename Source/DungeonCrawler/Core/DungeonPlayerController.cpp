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
#include "HealthComponent.h"
#include "CharacterClass.h"
#include "MainMenuWidget.h"
#include "LowHealthVignetteWidget.h"
#include "FpsCounterWidget.h"
#include "DungeonGameInstance.h"
#include "DungeonGenerator.h"
#include "BossArena.h"
#include "AmbientDust.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

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
		MinimapWidget = CreateWidget<UMinimapWidget>(this, UMinimapWidget::StaticClass());
		if (MinimapWidget)
		{
			MinimapWidget->AddToViewport(1);
		}
		// Low-health danger vignette (self-updating; sits under the menus).
		if (ULowHealthVignetteWidget* Vignette = CreateWidget<ULowHealthVignetteWidget>(this, ULowHealthVignetteWidget::StaticClass()))
		{
			Vignette->AddToViewport(2);
		}
		// Ambient dust motes drifting around the player (pooled; constant cost).
		if (UWorld* W = GetWorld())
		{
			FActorSpawnParameters DustParams;
			DustParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			W->SpawnActor<AAmbientDust>(AAmbientDust::StaticClass(), FTransform::Identity, DustParams);
		}
		// FPS counter, top-right under the minimap.
		if (UFpsCounterWidget* Fps = CreateWidget<UFpsCounterWidget>(this, UFpsCounterWidget::StaticClass()))
		{
			Fps->AddToViewport(1);
		}

		// First load of the session shows the start menu over the boot map; later loads (entering the
		// dungeon, returning to town, death restart) just fade in from black.
		UDungeonGameInstance* GI = GetGameInstance<UDungeonGameInstance>();
		if (GI && !GI->IsSessionStarted())
		{
			ShowMainMenu();
		}
		else
		{
			FadeFromBlack();
		}
	}
}

void ADungeonPlayerController::ShowMainMenu()
{
	if (MainMenuWidget)
	{
		return;
	}
	MainMenuWidget = CreateWidget<UMainMenuWidget>(this, UMainMenuWidget::StaticClass());
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(100); // above all gameplay UI
	}

	// Hold the camera black behind the menu so dismissing it reveals the world with a clean fade.
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.f, 1.f, 0.f, FLinearColor::Black, false, /*bHoldWhenFinished*/ true);
	}

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	if (APawn* P = GetPawn())
	{
		P->DisableInput(this); // freeze the player until they press Start
	}
}

void ADungeonPlayerController::StartGameFromMenu()
{
	if (UDungeonGameInstance* GI = GetGameInstance<UDungeonGameInstance>())
	{
		GI->SetSessionStarted();
	}
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (APawn* P = GetPawn())
	{
		P->EnableInput(this);
	}
	FadeFromBlack(0.6f);
}

void ADungeonPlayerController::StartGameAsClass(ECharacterClass Class)
{
	// Only apply the archetype to a brand-new character; a returning save keeps its existing build.
	bool bFresh = true;
	if (UDungeonGameInstance* GI = GetGameInstance<UDungeonGameInstance>())
	{
		bFresh = !GI->HasProfile();
	}
	if (bFresh)
	{
		if (AFirstPersonCharacter* FPChar = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			FPChar->ApplyClassLoadout(Class);
		}
	}
	StartGameFromMenu();
}

void ADungeonPlayerController::FadeFromBlack(float Duration)
{
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.f, 0.f, Duration, FLinearColor::Black, false, /*bHoldWhenFinished*/ false);
	}
}

// Two-step level transition: fade the camera to black now, then OpenLevel after the fade via a timer
// (see DoPendingTravel). PendingTravelMap doubles as the "in progress" latch so a second trigger (e.g.
// hitting the portal twice) during the fade is ignored.
void ADungeonPlayerController::FadeToBlackAndTravel(FName Map, float Duration)
{
	if (Map.IsNone() || !PendingTravelMap.IsNone())
	{
		return; // already traveling
	}
	PendingTravelMap = Map;

	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(0.f, 1.f, Duration, FLinearColor::Black, false, /*bHoldWhenFinished*/ true);
	}
	GetWorldTimerManager().SetTimer(TravelTimer, this, &ADungeonPlayerController::DoPendingTravel, FMath::Max(0.05f, Duration), false);
}

void ADungeonPlayerController::DoPendingTravel()
{
	if (!PendingTravelMap.IsNone())
	{
		UGameplayStatics::OpenLevel(this, PendingTravelMap);
	}
}

void ADungeonPlayerController::DevRevealMap()
{
	if (MinimapWidget)
	{
		MinimapWidget->RevealEntireMap();
	}
}

void ADungeonPlayerController::DevTeleportHome()
{
	FadeToBlackAndTravel(TEXT("L_Town"));
}

bool ADungeonPlayerController::DevToggleNoClip()
{
	if (AFirstPersonCharacter* P = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		P->SetNoClip(!P->IsNoClip());
		return P->IsNoClip();
	}
	return false;
}

bool ADungeonPlayerController::DevToggleGodMode()
{
	if (AFirstPersonCharacter* P = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UHealthComponent* H = P->GetHealthComponent())
		{
			H->SetInvulnerable(!H->IsInvulnerable());
			return H->IsInvulnerable();
		}
	}
	return false;
}

void ADungeonPlayerController::DevKill()
{
	if (AFirstPersonCharacter* P = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UHealthComponent* H = P->GetHealthComponent())
		{
			H->SetInvulnerable(false); // a dev kill always lands, even with god mode on
			H->Kill();
		}
	}
}

void ADungeonPlayerController::DevTeleportToBoss()
{
	UWorld* World = GetWorld();
	APawn* P = GetPawn();
	if (!World || !P)
	{
		return;
	}
	for (TActorIterator<ADungeonGenerator> It(World); It; ++It)
	{
		if (It->HasBossRoom())
		{
			const FVector Loc = It->GetBossRoomCenterWorld() + FVector(0.f, 0.f, 120.f);
			P->SetActorLocation(Loc, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
			// Teleporting skips the arena's entry trigger, so kick off the encounter directly (spawns the
			// boss, seals the doors, runs the intro). Otherwise the doors never close.
			for (TActorIterator<ABossArena> AIt(World); AIt; ++AIt)
			{
				AIt->ForceStart(P);
				break;
			}
			return;
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
		if (IsShopOpen()) { CloseShop(); } // don't stack the inventory on top of the shop
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
	// The shop has its own buy + sell lists, so close the standalone inventory to avoid overlapping panels.
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
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

// Central input-mode arbiter. Every panel open/close routes through here rather than setting the input
// mode itself, so the cursor + Game/UI mode are re-derived from whatever is *currently* open. This is why
// closing one of two stacked panels correctly leaves the cursor on (the other is still up) instead of
// snapping back to game-only. The pause menu is handled separately (it sets its own mode while paused).
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
