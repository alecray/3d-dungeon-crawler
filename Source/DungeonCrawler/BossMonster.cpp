#include "BossMonster.h"
#include "HealthComponent.h"
#include "Projectile.h"
#include "BubbleHazard.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

ABossMonster::ABossMonster()
{
	// Bigger, tankier, harder-hitting than a regular monster.
	GetCapsuleComponent()->InitCapsuleSize(75.f, 150.f);

	BodyScale = 2.2f;          // applied to BodyRoot in AMonsterCharacter::BeginPlay
	MonsterMaxHealth = 320.f;
	AttackDamage = 18.f;
	AttackRange = 260.f;
	AggroRange = 6000.f;       // notices the player anywhere in the big boss room
	AttackCooldown = 1.6f;
	MoveSpeed = 280.f;
	XPReward = 300;

	// Three cube "growths" revealed one per phase to show the boss morphing. Sized in BodyRoot's
	// un-scaled space (BodyRoot itself is scaled by BodyScale).
	MorphParts.Add(AddBox(TEXT("Morph1_BackSpike"), FVector(-28.f, 0.f, 45.f), FVector(28.f, 28.f, 70.f)));
	MorphParts.Add(AddBox(TEXT("Morph2_Crown"),     FVector(0.f, 0.f, 115.f), FVector(75.f, 75.f, 26.f)));
	MorphParts.Add(AddBox(TEXT("Morph3_Horns"),     FVector(34.f, 0.f, 70.f), FVector(40.f, 60.f, 80.f)));

	// Bright marker on the back (-X) flagging the phase-1 weak point; shown only during phase 1.
	BackWeakMesh = AddBox(TEXT("BackWeakPoint"), FVector(-32.f, 0.f, 55.f), FVector(20.f, 46.f, 46.f));

	// Face the player (not the movement direction) so the back weak point and lunges stay meaningful.
	GetCharacterMovement()->bOrientRotationToMovement = false;

	ProjectileClass = AProjectile::StaticClass();
	BubbleClass = ABubbleHazard::StaticClass();
}

void ABossMonster::BeginPlay()
{
	Super::BeginPlay();

	BaseMoveSpeed = MoveSpeed;

	// Hide every growth, then reveal phase 1's.
	for (UStaticMeshComponent* Part : MorphParts)
	{
		if (Part)
		{
			Part->SetVisibility(false);
		}
	}

	MoveState = EMoveState::Scuttle;
	MoveTimer = FMath::FRandRange(1.f, 2.f);
	StrafeSign = (FMath::FRand() < 0.5f) ? -1.f : 1.f;

	CurrentPhase = 0;
	AdvanceToPhase(1);
	ScheduleNextSpecial();
}

void ABossMonster::Tick(float DeltaSeconds)
{
	// Frozen during the spawn/intro animation: run only the intro, skip all chase/attack/special logic.
	if (bIntroPlaying)
	{
		UpdateIntro(DeltaSeconds);
		return;
	}

	Super::Tick(DeltaSeconds); // chase / melee / hit-react from the base monster

	if (!Health || Health->IsDead())
	{
		return;
	}

	// Phase transitions on health thresholds.
	const float Pct = Health->GetHealthPercent();
	if (CurrentPhase < 3 && Pct <= Phase3HealthPct)
	{
		AdvanceToPhase(3);
	}
	else if (CurrentPhase < 2 && Pct <= Phase2HealthPct)
	{
		AdvanceToPhase(2);
	}

	const float Now = GetWorld()->GetTimeSeconds();

	// End an enrage speed-burst when it expires.
	if (EnrageEndTime > 0.f && Now >= EnrageEndTime)
	{
		EnrageEndTime = 0.f;
		GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
	}

	// Fire a random special on the timer.
	if (Now >= NextSpecialTime)
	{
		DoRandomSpecial();
		ScheduleNextSpecial();
	}
}

void ABossMonster::PlayIntro()
{
	bIntroPlaying = true;
	IntroTimeLeft = FMath::Max(0.1f, IntroDuration);

	// Stop any movement and start the body compressed so it can rise/roar out of the ground.
	GetCharacterMovement()->StopMovementImmediately();
	if (BodyRoot)
	{
		BodyRoot->SetRelativeScale3D(FVector(BodyScale * 0.2f));
	}
}

void ABossMonster::UpdateIntro(float DeltaSeconds)
{
	IntroTimeLeft = FMath::Max(0.f, IntroTimeLeft - DeltaSeconds);
	const float A = 1.f - (IntroDuration > 0.f ? IntroTimeLeft / IntroDuration : 0.f); // 0 -> 1

	if (BodyRoot)
	{
		// Ease up to full size over the first 70%, then a brief overshoot "roar" wobble to settle.
		float Scale = FMath::InterpEaseOut(0.2f, 1.f, FMath::Clamp(A / 0.7f, 0.f, 1.f), 2.f);
		if (A > 0.7f)
		{
			Scale += FMath::Sin((A - 0.7f) / 0.3f * PI) * 0.12f;
		}
		BodyRoot->SetRelativeScale3D(FVector(BodyScale * Scale));
	}

	if (IntroTimeLeft <= 0.f)
	{
		bIntroPlaying = false;
		if (BodyRoot)
		{
			BodyRoot->SetRelativeScale3D(FVector(BodyScale));
		}
		ScheduleNextSpecial(); // start the specials clock fresh now that the fight begins
	}
}

void ABossMonster::AdvanceToPhase(int32 NewPhase)
{
	CurrentPhase = NewPhase;

	if (MorphParts.IsValidIndex(NewPhase - 1) && MorphParts[NewPhase - 1])
	{
		MorphParts[NewPhase - 1]->SetVisibility(true);
	}

	// The back weak point is only exposed in phase 1; the boss armors over it afterwards.
	bBackWeakActive = (NewPhase == 1);
	if (BackWeakMesh)
	{
		BackWeakMesh->SetVisibility(bBackWeakActive);
	}

	TriggerHitReact(); // visual pop on morph

	// Phases 2 and 3 buff the boss.
	if (NewPhase >= 2)
	{
		AttackDamage *= 1.25f;
		MoveSpeed += 40.f;
		BaseMoveSpeed = MoveSpeed;
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		AttackCooldown = FMath::Max(0.5f, AttackCooldown * 0.85f);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 10, 3.f, FColor::Orange,
			FString::Printf(TEXT("THE BOSS ENTERS PHASE %d!"), NewPhase));
	}
}

void ABossMonster::ScheduleNextSpecial()
{
	// Specials come faster in later phases.
	const float Scale = 1.f / (1.f + 0.25f * (CurrentPhase - 1));
	const float Delay = FMath::FRandRange(SpecialMinInterval, SpecialMaxInterval) * Scale;
	NextSpecialTime = GetWorld()->GetTimeSeconds() + Delay;
}

void ABossMonster::DoRandomSpecial()
{
	// Pool of mechanics widens with phase.
	TArray<int32, TInlineAllocator<5>> Pool;
	Pool.Add(0); // slam (always)
	Pool.Add(3); // projectile volley (always)
	if (CurrentPhase >= 2) { Pool.Add(1); } // summon
	if (CurrentPhase >= 2) { Pool.Add(4); } // bubble burst
	if (CurrentPhase >= 3) { Pool.Add(2); } // enrage

	switch (Pool[FMath::RandRange(0, Pool.Num() - 1)])
	{
	case 0: SlamAttack(); break;
	case 1: SummonAdds(); break;
	case 2: EnrageBurst(); break;
	case 3: SpitProjectiles(); break;
	case 4: BubbleBurst(); break;
	default: break;
	}
}

void ABossMonster::SlamAttack()
{
	TriggerHitReact();

	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		ToPlayer.Z = 0.f;
		if (ToPlayer.SizeSquared() <= SlamRadius * SlamRadius)
		{
			if (UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>())
			{
				PlayerHealth->ApplyDamage(SlamDamage);
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 11, 1.5f, FColor::Red, TEXT("Boss: GROUND SLAM!"));
	}
}

void ABossMonster::SummonAdds()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < SummonCount; ++i)
	{
		const FVector Offset(FMath::FRandRange(-300.f, 300.f), FMath::FRandRange(-300.f, 300.f), 100.f);
		World->SpawnActor<AMonsterCharacter>(AMonsterCharacter::StaticClass(),
			FTransform(GetActorLocation() + Offset), Params);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 12, 1.5f, FColor::Purple, TEXT("Boss: SUMMONS MINIONS!"));
	}
}

void ABossMonster::EnrageBurst()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * 2.f;
	EnrageEndTime = GetWorld()->GetTimeSeconds() + 3.f;
	TriggerHitReact();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 13, 1.5f, FColor::Yellow, TEXT("Boss: ENRAGED!"));
	}
}

void ABossMonster::SpitProjectiles()
{
	UWorld* World = GetWorld();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!World || !ProjectileClass || !Player)
	{
		return;
	}

	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 120.f + FVector(0.f, 0.f, 120.f);
	FVector ToPlayer = (Player->GetActorLocation() + FVector(0.f, 0.f, 40.f)) - Muzzle;
	if (!ToPlayer.Normalize())
	{
		return;
	}

	// One aimed bolt in phase 1; a 3-bolt spread from phase 2 on.
	const int32 Count = (CurrentPhase >= 2) ? 3 : 1;

	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < Count; ++i)
	{
		const float Yaw = (Count > 1) ? FMath::Lerp(-ProjectileSpread, ProjectileSpread, (float)i / (Count - 1)) : 0.f;
		const FVector Dir = ToPlayer.RotateAngleAxis(Yaw, FVector::UpVector);
		if (AProjectile* Proj = World->SpawnActor<AProjectile>(ProjectileClass, FTransform(Dir.Rotation(), Muzzle), P))
		{
			Proj->Launch(Dir, ProjectileDamage, this, /*bTargetPlayer*/ true, /*GravityScale*/ 0.f);
		}
	}

	TriggerHitReact();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 14, 1.5f, FColor::Cyan, TEXT("Boss: SPITS PROJECTILES!"));
	}
}

void ABossMonster::BubbleBurst()
{
	UWorld* World = GetWorld();
	if (!World || !BubbleClass)
	{
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Around = Player ? Player->GetActorLocation() : GetActorLocation();

	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Drop a cluster of pools around the player, forcing them to reposition.
	for (int32 i = 0; i < BubbleCount; ++i)
	{
		const FVector Offset(FMath::FRandRange(-360.f, 360.f), FMath::FRandRange(-360.f, 360.f), 0.f);
		FVector Loc = Around + Offset;
		Loc.Z = Around.Z - 70.f; // sit roughly on the floor
		if (ABubbleHazard* Bubble = World->SpawnActor<ABubbleHazard>(BubbleClass, FTransform(Loc), P))
		{
			Bubble->Init(BubbleRadius, BubbleDamage, BubbleLifetime);
		}
	}

	TriggerHitReact();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 15, 1.5f, FColor::Green, TEXT("Boss: BUBBLING POOLS ERUPT!"));
	}
}

float ABossMonster::ApplyHitDamage(float BaseDamage, const FVector& FromLocation)
{
	float Dmg = BaseDamage;
	bool bWeak = false;

	if (bBackWeakActive)
	{
		FVector ToHit = FromLocation - GetActorLocation();
		ToHit.Z = 0.f;
		// A hit counts as a back strike when it comes from behind the boss's facing.
		if (ToHit.Normalize() && FVector::DotProduct(GetActorForwardVector(), ToHit) < -0.25f)
		{
			Dmg *= WeakPointMultiplier;
			bWeak = true;
		}
	}

	bLastHitWeak = bWeak; // read by HandleDamaged to style the floating number
	return Health ? Health->ApplyDamage(Dmg) : 0.f;
}

bool ABossMonster::HasLineOfSightToPlayer(AActor* Player) const
{
	UWorld* World = GetWorld();
	if (!World || !Player)
	{
		return false;
	}

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector End = Player->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BossLoS), /*bTraceComplex*/ false, this);
	Params.AddIgnoredActor(Player);
	// A blocking hit on anything other than the player (i.e. a wall) means no line of sight.
	return !World->LineTraceTestByChannel(Start, End, ECC_Visibility, Params);
}

bool ABossMonster::TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist)
{
	// Always face the player so the back weak point and forward lunges stay meaningful.
	const FRotator Face(0.f, DirToPlayer.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 7.f));

	// Hybrid pathing: when a wall blocks the player, hand off to the base navmesh chase to route around
	// it; only scuttle/lunge while we have a clear line of sight (the usual case in the open arena).
	if (!HasLineOfSightToPlayer(Player))
	{
		bUsingDirectMove = false;
		return false; // base AMonsterCharacter::Tick runs its navmesh MoveToActor chase
	}
	if (!bUsingDirectMove)
	{
		// Just regained sight: cancel any active path-follow so our direct steering takes over cleanly.
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		bUsingDirectMove = true;
	}

	const FVector Lateral = FVector::CrossProduct(FVector::UpVector, DirToPlayer).GetSafeNormal();
	MoveTimer -= DeltaSeconds;

	switch (MoveState)
	{
	case EMoveState::Scuttle:
	{
		// Sidestep around the player while drifting slowly inward — a crab-like scuttle.
		const FVector Move = (Lateral * StrafeSign * 0.85f + DirToPlayer * 0.35f).GetSafeNormal();
		AddMovementInput(Move, 1.f);
		if (MoveTimer <= 0.f)
		{
			if (FMath::FRand() < 0.5f) { StrafeSign *= -1.f; } // occasionally reverse the circle
			if (Dist < LungeRange)
			{
				MoveState = EMoveState::Telegraph;
				MoveTimer = LungeTelegraph;
				TriggerHitReact(); // wind-up tell
			}
			else
			{
				MoveTimer = FMath::FRandRange(0.8f, 1.6f);
			}
		}
		break;
	}
	case EMoveState::Telegraph:
		// Hold still, faced at the player, then dash.
		if (MoveTimer <= 0.f)
		{
			MoveState = EMoveState::Lunge;
			MoveTimer = LungeTime;
			GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * LungeSpeedMult;
		}
		break;
	case EMoveState::Lunge:
		AddMovementInput(DirToPlayer, 1.f); // fast forward dash
		if (MoveTimer <= 0.f)
		{
			MoveState = EMoveState::Scuttle;
			MoveTimer = FMath::FRandRange(1.2f, 2.2f);
			StrafeSign = (FMath::FRand() < 0.5f) ? -1.f : 1.f;
			// Restore normal speed (respecting an active enrage burst).
			GetCharacterMovement()->MaxWalkSpeed = (EnrageEndTime > 0.f) ? BaseMoveSpeed * 2.f : BaseMoveSpeed;
		}
		break;
	}

	return true; // boss handles its own chase movement
}
