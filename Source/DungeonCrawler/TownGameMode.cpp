#include "TownGameMode.h"
#include "BlackjackTable.h"

#include "EngineUtils.h"                 // TActorIterator
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"

void ATownGameMode::BuildWorld()
{
	// Town is a saved level; just make sure it has the stylized lighting/grade. The player spawns at
	// the map's PlayerStart (no dungeon, no pawn relocation).
	EnsureLighting();
	EnsurePostProcess();

	// Prototype: code-spawn a 3D blackjack table near the player start so it's reachable without editing
	// the map. Reposition it in L_Town later (and drop this spawn) once it has a permanent home.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector StartLoc = FVector::ZeroVector;
	FRotator StartRot = FRotator::ZeroRotator;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		StartLoc = It->GetActorLocation();
		StartRot = It->GetActorRotation();
		break;
	}

	// A few metres in front of the player start, then dropped onto the floor.
	const FVector Ahead = StartLoc + StartRot.Vector() * 360.f + FVector(0.f, 0.f, 150.f);
	FVector TableLoc(Ahead.X, Ahead.Y, StartLoc.Z - 88.f); // fallback ~floor
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Ahead, Ahead - FVector(0.f, 0.f, 1000.f), ECC_Visibility))
	{
		TableLoc.Z = Hit.ImpactPoint.Z;
	}

	// Face the table back toward the player start so its readout faces the approach.
	const FRotator TableRot(0.f, StartRot.Yaw + 180.f, 0.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World->SpawnActor<ABlackjackTable>(ABlackjackTable::StaticClass(), FTransform(TableRot, TableLoc), Params);
}
