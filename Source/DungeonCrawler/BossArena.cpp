#include "BossArena.h"
#include "BossMonster.h"
#include "BossDoor.h"
#include "Portal.h"
#include "BossIntroCameraShake.h"
#include "BossHealthBarWidget.h"
#include "HealthComponent.h"
#include "DungeonGameInstance.h"
#include "ItemPickup.h"
#include "ItemTypes.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

ABossArena::ABossArena()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(600.f, 600.f, 250.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ABossArena::OnTriggerOverlap);
}

void ABossArena::Configure(TSubclassOf<ABossMonster> InBossClass, const FVector& RoomCenterWorld, const FVector2D& RoomHalfXY,
	TSubclassOf<APortal> InPortalClass, FName InTownMap)
{
	BossClass = InBossClass;
	PortalClass = InPortalClass;
	TownMapName = InTownMap;
	RoomCenter = RoomCenterWorld;
	RoomHalf = RoomHalfXY;

	// Sit the trigger over the middle of the room so the player is WELL inside (clear of the entrance
	// doorway) before the fight starts and the door seals behind them — otherwise the rising door can
	// catch the player on the threshold and lock them out.
	SetActorLocation(RoomCenter + FVector(0.f, 0.f, 200.f));
	if (Trigger)
	{
		Trigger->SetBoxExtent(FVector(FMath::Max(100.f, RoomHalf.X * 0.55f), FMath::Max(100.f, RoomHalf.Y * 0.55f), 250.f));
	}
}

void ABossArena::AddDoorSlot(const FTransform& WorldXf, const FVector& SizeCm)
{
	DoorSlots.Add({ WorldXf, SizeCm });
}

void ABossArena::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The boss, doors and return portal are spawned at runtime and owned by this arena (not tracked by
	// the generator), so destroy them here — otherwise a dungeon regenerate leaves orphaned bosses around.
	if (Boss) { Boss->Destroy(); }
	for (ABossDoor* Door : Doors) { if (Door) { Door->Destroy(); } }
	for (AItemPickup* Drop : LootDrops) { if (Drop) { Drop->Destroy(); } }
	if (ReturnPortal) { ReturnPortal->Destroy(); }
	if (IntroCamera) { IntroCamera->Destroy(); }
	if (HealthBar) { HealthBar->RemoveFromParent(); HealthBar = nullptr; }

	Super::EndPlay(EndPlayReason);
}

void ABossArena::OnTriggerOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	if (bStarted || !OtherActor)
	{
		return;
	}
	// Only the player triggers the encounter.
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (Pawn->IsPlayerControlled())
		{
			StartEncounter(Pawn);
		}
	}
}

void ABossArena::ForceStart(APawn* Player)
{
	StartEncounter(Player); // StartEncounter no-ops if already started
}

void ABossArena::StartEncounter(APawn* Player)
{
	UWorld* World = GetWorld();
	if (bStarted || !World || !BossClass || !Player)
	{
		return;
	}
	bStarted = true;

	// --- Spawn the boss on the far side of the room, opposite the player, facing them. ---
	FVector AwayFromPlayer = RoomCenter - Player->GetActorLocation();
	AwayFromPlayer.Z = 0.f;
	if (!AwayFromPlayer.Normalize())
	{
		AwayFromPlayer = FVector(1.f, 0.f, 0.f);
	}
	const float Reach = FMath::Max(RoomHalf.X, RoomHalf.Y) * 0.7f;
	const FVector BossLoc = RoomCenter + AwayFromPlayer * Reach + FVector(0.f, 0.f, 160.f);
	const FRotator BossRot = (-AwayFromPlayer).Rotation(); // face back toward the player

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Boss = World->SpawnActor<ABossMonster>(BossClass, FTransform(BossRot, BossLoc), Params);
	if (!Boss)
	{
		return;
	}
	if (UHealthComponent* BossHealth = Boss->FindComponentByClass<UHealthComponent>())
	{
		BossHealth->OnDepleted.AddUObject(this, &ABossArena::OnBossDefeated);
	}

	// Boss health bar pinned to the top of the screen for the fight.
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		HealthBar = CreateWidget<UBossHealthBarWidget>(PC, UBossHealthBarWidget::StaticClass());
		if (HealthBar)
		{
			HealthBar->SetBoss(Boss);
			HealthBar->AddToViewport(5);
		}
	}

	// --- Seal the entrance doors. Spawn with AlwaysSpawn (NOT the boss's AdjustIfPossible params): the
	//     door cube starts overlapping the floor/walls, so "adjust" would shove it off the doorway and it
	//     would never seal. ---
	FActorSpawnParameters DoorParams;
	DoorParams.Owner = this;
	DoorParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FDoorSlot& Slot : DoorSlots)
	{
		if (ABossDoor* Door = World->SpawnActor<ABossDoor>(ABossDoor::StaticClass(), Slot.Xf, DoorParams))
		{
			Door->Init(Slot.Size);
			Door->Seal();
			Doors.Add(Door);
		}
	}

	// --- Dormant return portal near the room center (activates on the boss's death). ---
	TSubclassOf<APortal> PClass = PortalClass;
	if (!PClass) { PClass = APortal::StaticClass(); }
	const FVector PortalLoc = RoomCenter + FVector(0.f, FMath::Max(150.f, RoomHalf.Y * 0.4f), 0.f);
	ReturnPortal = World->SpawnActor<APortal>(PClass, FTransform(FRotator::ZeroRotator, PortalLoc), Params);
	if (ReturnPortal)
	{
		ReturnPortal->SetTargetMapName(TownMapName);
		ReturnPortal->SetActive(false);
	}

	// --- The boss always plays its spawn/intro animation + the camera/shake cinematic, every time
	//     (no longer gated on a first-time-per-boss save flag — replays get the full intro too). ---
	Boss->PlayIntro();
	RunIntroCinematic(Player);
}

void ABossArena::RunIntroCinematic(APawn* Player)
{
	UWorld* World = GetWorld();
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!World || !PC || !Boss)
	{
		return;
	}

	// Lock the player out for the duration of the cinematic.
	Player->DisableInput(PC);

	// Frame the boss straight-on. Keep the shot LOW and INSIDE the room: aim at a sensible mid-body height
	// (capped so a huge boss doesn't drag the focus way up), pull back proportional to the boss but clamp
	// to the room size so the camera never slides outside/over the walls, and only rise a little — earlier
	// the height + pullback scaled with the boss and put the camera up above the level.
	FVector ToPlayer = Player->GetActorLocation() - Boss->GetActorLocation();
	ToPlayer.Z = 0.f;
	ToPlayer.Normalize();
	const float HalfH = Boss->GetCapsuleComponent() ? Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 100.f;
	const float FloorZ = Boss->GetActorLocation().Z - HalfH;
	const FVector Focus(Boss->GetActorLocation().X, Boss->GetActorLocation().Y, FloorZ + FMath::Min(HalfH, 200.f));
	const float RoomMin = FMath::Min(RoomHalf.X, RoomHalf.Y);
	const float Dist = FMath::Clamp(HalfH * 2.2f, 420.f, FMath::Max(450.f, RoomMin * 1.15f));
	const FVector CamLoc = Focus + ToPlayer * Dist + FVector(0.f, 0.f, 70.f); // small, fixed rise

	FActorSpawnParameters Params;
	Params.Owner = this;
	IntroCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(CamLoc), Params);
	if (IntroCamera)
	{
		IntroCamera->SetActorRotation((Focus - CamLoc).Rotation());
		PC->SetViewTargetWithBlend(IntroCamera, 0.5f);
	}

	// Screen shake for the boss's roar.
	PC->ClientStartCameraShake(UBossIntroCameraShake::StaticClass(), 1.f);

	// Hold on the boss for its intro, then blend back to the player and restore control.
	const float IntroHold = FMath::Max(0.6f, Boss->IsIntroPlaying() ? 1.8f : 1.2f);
	World->GetTimerManager().SetTimer(CameraReturnTimer, this, &ABossArena::ReturnCameraToPlayer, IntroHold, false);
}

void ABossArena::ReturnCameraToPlayer()
{
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(Pawn, 0.5f);
		}
	}
	if (World)
	{
		World->GetTimerManager().SetTimer(FinishTimer, this, &ABossArena::FinishCinematic, 0.5f, false);
	}
	else
	{
		FinishCinematic();
	}
}

void ABossArena::FinishCinematic()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->EnableInput(PC); // hand control back; the fight is on
		}
	}
	if (IntroCamera)
	{
		IntroCamera->Destroy();
		IntroCamera = nullptr;
	}
}

void ABossArena::OnBossDefeated(UHealthComponent* /*DeadComponent*/)
{
	// Where the boss fell — loot drops here, at FLOOR level (the boss's origin is at its capsule centre,
	// which for the big crab is high up; dropping there put the loot in the ceiling).
	FVector DeathLoc = RoomCenter;
	if (IsValid(Boss))
	{
		const float CapHalf = Boss->GetCapsuleComponent() ? Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		DeathLoc = Boss->GetActorLocation() - FVector(0.f, 0.f, CapHalf);
	}

	// Open the entrance doors.
	for (ABossDoor* Door : Doors)
	{
		if (Door) { Door->Open(); }
	}

	// Scatter the boss's loot at the kill site.
	DropBossLoot(DeathLoc);

	// Activate the return portal AWAY from the loot: drop it at whichever back corner of the room is
	// farthest from where the boss died, so the reward and the exit don't pile up on the same spot. (The
	// portal was hidden while dormant, so repositioning it now causes no visible pop.)
	if (ReturnPortal)
	{
		FVector Best = RoomCenter;
		float BestDistSq = -1.f;
		for (int32 sx = -1; sx <= 1; sx += 2)
		{
			for (int32 sy = -1; sy <= 1; sy += 2)
			{
				const FVector Cand = RoomCenter + FVector(sx * RoomHalf.X * 0.6f, sy * RoomHalf.Y * 0.6f, 0.f);
				const float D = FVector::DistSquared2D(Cand, DeathLoc);
				if (D > BestDistSq) { BestDistSq = D; Best = Cand; }
			}
		}
		ReturnPortal->SetActorLocation(FVector(Best.X, Best.Y, RoomCenter.Z + 60.f));
		ReturnPortal->SetActive(true);
	}

	if (HealthBar)
	{
		HealthBar->RemoveFromParent();
		HealthBar = nullptr;
	}
}

void ABossArena::DropBossLoot(const FVector& DeathLoc)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FRandomStream Rng(FMath::Rand());
	const int32 Drops = Rng.RandRange(3, 5);
	for (int32 i = 0; i < Drops; ++i)
	{
		// Boss loot is "best of 2" rolls so it skews a little better than ordinary chest loot.
		FName ItemId = ItemDatabase::RollRandomItem(Rng);
		const FName Alt = ItemDatabase::RollRandomItem(Rng);
		if (ItemId.IsNone() || (!Alt.IsNone() && ItemDatabase::Get(Alt).Rarity > ItemDatabase::Get(ItemId).Rarity))
		{
			ItemId = Alt;
		}
		if (ItemId.IsNone())
		{
			continue;
		}

		// Scatter in a ring around the kill site.
		const float Ang = Rng.FRandRange(0.f, 2.f * PI);
		const float Rad = Rng.FRandRange(70.f, 230.f);
		const FVector Loc = DeathLoc + FVector(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad, 30.f);

		if (AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FTransform(Loc), Params))
		{
			const int32 Count = (ItemDatabase::Get(ItemId).MaxStack > 1) ? Rng.RandRange(1, 3) : 1;
			Pickup->Configure(ItemId, Count);
			LootDrops.Add(Pickup);
		}
	}
}
