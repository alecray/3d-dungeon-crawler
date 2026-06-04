#include "TownGameMode.h"
#include "BlackjackTable.h"
#include "FishingHole.h"

#include "EngineUtils.h"                 // TActorIterator
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"

void ATownGameMode::BuildWorld()
{
	// Town is a saved level; just make sure it has the stylized lighting/grade. The player spawns at
	// the map's PlayerStart (no dungeon, no pawn relocation).
	EnsureLighting();
	EnsurePostProcess();

	// Prototype: code-spawn the town minigame stations near the player start so they're reachable without
	// editing the map. Reposition them in L_Town later (and drop these spawns) once they have homes.
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

	const FVector Fwd = StartRot.Vector();
	FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();

	// Drops a station onto the floor at an offset (forward, sideways) from the player start, facing the
	// approach (+ an optional extra yaw to orient a particular station).
	auto SpawnStation = [&](UClass* Class, float FwdCm, float RightCm, float ExtraYaw)
	{
		const FVector Probe = StartLoc + Fwd * FwdCm + Right * RightCm + FVector(0.f, 0.f, 150.f);
		FVector Loc(Probe.X, Probe.Y, StartLoc.Z - 88.f); // fallback ~floor
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Probe, Probe - FVector(0.f, 0.f, 1000.f), ECC_Visibility))
		{
			Loc.Z = Hit.ImpactPoint.Z;
		}
		const FRotator Rot(0.f, StartRot.Yaw + 180.f + ExtraYaw, 0.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		World->SpawnActor<AActor>(Class, FTransform(Rot, Loc), Params);
	};

	SpawnStation(ABlackjackTable::StaticClass(), 360.f, -250.f, /*ExtraYaw*/ 180.f); // ahead-left, flipped to face the player
	SpawnStation(AFishingHole::StaticClass(),    360.f,  250.f, /*ExtraYaw*/ 0.f);   // ahead-right
}
