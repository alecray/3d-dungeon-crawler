#include "BossArena.h"
#include "BossMonster.h"
#include "BossDoor.h"
#include "Portal.h"
#include "BossIntroCameraShake.h"
#include "BossHealthBarWidget.h"
#include "HealthComponent.h"
#include "DungeonGameInstance.h"

#include "Components/BoxComponent.h"
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

	// Sit the trigger over the room, a little inside the walls so the player is well inside when it fires.
	SetActorLocation(RoomCenter + FVector(0.f, 0.f, 200.f));
	if (Trigger)
	{
		Trigger->SetBoxExtent(FVector(FMath::Max(100.f, RoomHalf.X * 0.85f), FMath::Max(100.f, RoomHalf.Y * 0.85f), 250.f));
	}
}

void ABossArena::AddDoorSlot(const FTransform& WorldXf, const FVector& SizeCm)
{
	DoorSlots.Add({ WorldXf, SizeCm });
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

	// --- Seal the entrance doors. ---
	for (const FDoorSlot& Slot : DoorSlots)
	{
		if (ABossDoor* Door = World->SpawnActor<ABossDoor>(ABossDoor::StaticClass(), Slot.Xf, Params))
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

	// --- The boss always plays its spawn/intro animation. ---
	Boss->PlayIntro();

	// --- Cinematic only the first time the player ever fights this boss type. ---
	UDungeonGameInstance* GI = GetGameInstance<UDungeonGameInstance>();
	const bool bFirstTime = !GI || !GI->HasSeenBossIntro(Boss->GetBossId());
	if (bFirstTime)
	{
		if (GI) { GI->MarkBossIntroSeen(Boss->GetBossId()); }
		RunIntroCinematic(Player);
	}
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

	// Frame the boss from a point between it and the player, slightly raised.
	FVector ToPlayer = Player->GetActorLocation() - Boss->GetActorLocation();
	ToPlayer.Z = 0.f;
	ToPlayer.Normalize();
	const FVector Focus = Boss->GetActorLocation() + FVector(0.f, 0.f, 120.f);
	const FVector CamLoc = Focus + ToPlayer * 380.f + FVector(0.f, 0.f, 160.f);

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
	// Open the entrance doors and turn on the return portal.
	for (ABossDoor* Door : Doors)
	{
		if (Door) { Door->Open(); }
	}
	if (ReturnPortal)
	{
		ReturnPortal->SetActive(true);
	}
	if (HealthBar)
	{
		HealthBar->RemoveFromParent();
		HealthBar = nullptr;
	}
}
