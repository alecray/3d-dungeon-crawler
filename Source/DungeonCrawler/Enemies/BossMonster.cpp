#include "BossMonster.h"
#include "HealthComponent.h"
#include "Projectile.h"
#include "BubbleHazard.h"
#include "MonsterTypes.h"
#include "BossSpawnVFX.h"
#include "AttackTelegraph.h"

#include "AIController.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	AttackDamage = 135.f;       // a brutal hit (~75% of the 180 starting HP) — Dark-Souls punishing
	AttackRange = 380.f;        // wide danger zone — you must DODGE out of it, a step back won't clear it
	                            // (the red telegraph disc matches this reach exactly)
	AggroRange = 6000.f;       // notices the player anywhere in the big boss room
	AttackCooldown = 5.6f;      // long recovery window between swings — big punish openings
	AttackHitFrame = 20.f;      // the claw connects on frame 20 of the attack anim (dodgeable until then)
	MoveSpeed = 430.f;          // closes on the player quickly (was a sluggish 280)
	XPReward = 300;

	// Save the originals so SetTier can scale from them rather than re-scaling an already-scaled value.
	BaseMonsterMaxHealth = MonsterMaxHealth;
	BaseAttackDamage = AttackDamage;

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

// ---------------------------------------------------------------------------
// Tier system
// ---------------------------------------------------------------------------

void ABossMonster::SetTier(int32 InTier)
{
	Tier = FMath::Clamp(InTier, 0, 3);

	// Stat scaling: multiply base stats by (1 + TierStatScale * Tier).
	// Tier 0 => scale = 1.0 (unchanged). Tier 1 => 1.5x. Tier 2 => 2.0x. Tier 3 => 2.5x (default 0.5).
	const float Scale = 1.f + TierStatScale * (float)Tier;
	MonsterMaxHealth = BaseMonsterMaxHealth * Scale;
	AttackDamage = BaseAttackDamage * Scale;

	// Tier 0 = 2 lives. Higher tiers can have more (reserved for future design).
	// TODO(tier1+): tune MaxPhases and unlock additional phase-ability pools per tier.
	MaxPhases = (Tier == 0) ? 2 : (2 + Tier); // tier 1->3, tier 2->4, tier 3->5 (placeholder)
	PhasesRemaining = MaxPhases;

	// Apply the new max health immediately if HealthComponent already exists (called post-BeginPlay
	// from the arena). If it doesn't exist yet, BeginPlay will call Health->SetMaxHealth itself.
	if (Health)
	{
		Health->SetMaxHealth(MonsterMaxHealth, /*bRefill*/ true);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 8, 4.f, FColor::Orange,
			FString::Printf(TEXT("Boss tier %d: HP=%.0f ATK=%.0f MaxPhases=%d"), Tier, MonsterMaxHealth, AttackDamage, MaxPhases));
	}
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void ABossMonster::BeginPlay()
{
	Super::BeginPlay();

	BaseMoveSpeed = MoveSpeed;

	// Set up the animated skeletal crab body (hides the graybox cubes on success; if the rig fails to load,
	// the graybox cubes remain as a last-resort fallback).
	const bool bSkeletal = SetupSkeletalBody(SkeletalMeshPath, SkeletalMeshScale, RunAnimPath, IdleAnimPath, AttackAnimPath,
		DeathAnimPath, FlinchAnimPath);

	// Give the boss a subtle self-glow so the big crab reads in the dark dungeon (drives M_Base emissive).
	SetBodyEmissive(0.35f);

	// Resizing the capsule to fit the crab can leave it clipping into the floor — and because the boss is
	// frozen during its intro it can't settle via gravity. Snap it so the capsule base rests on the floor.
	if (bSkeletal)
	{
		if (UWorld* World = GetWorld())
		{
			const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FVector Center = GetActorLocation();
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(BossFloorSnap), /*bTraceComplex*/ false, this);
			if (World->LineTraceSingleByChannel(Hit, Center, Center - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Params))
			{
				SetActorLocation(FVector(Center.X, Center.Y, Hit.ImpactPoint.Z + HalfHeight + 2.f),
					/*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

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

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void ABossMonster::Tick(float DeltaSeconds)
{
	// Frozen during the spawn/intro animation: run only the intro, skip all chase/attack/special logic.
	if (bIntroPlaying)
	{
		UpdateIntro(DeltaSeconds);
		return;
	}

	// Tucked into its shell: immobile + invulnerable until it re-emerges.
	if (bShellRetreat)
	{
		UpdateShellRetreat(DeltaSeconds);
		return;
	}

	Super::Tick(DeltaSeconds); // chase / melee / hit-react from the base monster

	if (!Health || Health->IsDead())
	{
		return;
	}

	// ---- Slam wind-up detonation ----
	if (bSlamPending)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now >= SlamDetonateTime)
		{
			bSlamPending = false;

			// Deal damage only if the player is GROUNDED (not airborne). Jump to dodge!
			if (APawn* Player = GetWorld()->GetFirstPlayerController()
				? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
			{
				FVector ToPlayer = Player->GetActorLocation() - SlamCenter;
				ToPlayer.Z = 0.f;
				if (ToPlayer.SizeSquared() <= SlamRadius * SlamRadius)
				{
					// Check airborne state via CharacterMovementComponent.
					bool bPlayerGrounded = true;
					if (UCharacterMovementComponent* PMC = Player->FindComponentByClass<UCharacterMovementComponent>())
					{
						bPlayerGrounded = !PMC->IsFalling();
					}

					if (bPlayerGrounded)
					{
						if (UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>())
						{
							PlayerHealth->ApplyDamage(SlamDamage);
						}
						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(/*Key*/ 11, 1.5f, FColor::Red, TEXT("Boss: GROUND SLAM HIT!"));
						}
					}
					else
					{
						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(/*Key*/ 11, 1.5f, FColor::Green, TEXT("Boss: GROUND SLAM — player jumped, EVADED!"));
						}
					}
				}
			}

			// Clear the telegraph reference (it auto-destroys via lifespan, but null it so we don't hold a stale ptr).
			SlamTelegraph = nullptr;
		}
	}

	// ---- End enrage speed-burst when it expires ----
	const float Now = GetWorld()->GetTimeSeconds();
	if (EnrageEndTime > 0.f && Now >= EnrageEndTime)
	{
		EnrageEndTime = 0.f;
		GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
	}

	// ---- Fire a random special on the timer ----
	if (Now >= NextSpecialTime)
	{
		DoRandomSpecial();
		ScheduleNextSpecial();
	}
}

// ---------------------------------------------------------------------------
// Specials scheduling
// ---------------------------------------------------------------------------

void ABossMonster::ScheduleNextSpecial()
{
	// Specials come faster in later phases.
	const float Scale = 1.f / (1.f + 0.25f * (CurrentPhase - 1));
	const float Delay = FMath::FRandRange(SpecialMinInterval, SpecialMaxInterval) * Scale;
	NextSpecialTime = GetWorld()->GetTimeSeconds() + Delay;
}

void ABossMonster::DoRandomSpecial()
{
	// Tier 0: phase 1 = no specials (scuttle/lunge/melee only); phase 2+ = ground slam.
	// Tier 1+: reserved pools below. Add ability content and un-gate them in a future pass.
	// TODO(tier1+): design and enable projectile volley, summon adds, enrage, bubble pools, shell-retreat.

	if (Tier == 0)
	{
		if (CurrentPhase >= 2)
		{
			SlamAttack(); // the only special enabled in tier 0 phase 2
		}
		// Phase 1: no specials — pure scuttle/lunge + melee.
		return;
	}

	// ---- Tier 1+ ability pool (stat-scaled fights, ability content deferred) ----
	// TODO(tier1+): replace these stubs with real ability design once tier 1+ content is authored.
	if (CurrentPhase < 2)
	{
		return; // phase 1 on tier 1+: no specials yet
	}

	TArray<int32, TInlineAllocator<6>> Pool; // reached only in phase 2+
	Pool.Add(0); // slam
	Pool.Add(3); // projectile volley
	Pool.Add(1); // summon
	Pool.Add(4); // bubble burst
	if (CurrentPhase >= 3) { Pool.Add(2); } // enrage
	if (CurrentPhase >= 3) { Pool.Add(5); } // shell retreat

	switch (Pool[FMath::RandRange(0, Pool.Num() - 1)])
	{
	case 0: SlamAttack(); break;
	case 1: SummonAdds(); break;
	case 2: EnrageBurst(); break;
	case 3: SpitProjectiles(); break;
	case 4: BubbleBurst(); break;
	case 5: EnterShellRetreat(); break;
	default: break;
	}
}

// ---------------------------------------------------------------------------
// Ground Slam — dodge by JUMPING
// ---------------------------------------------------------------------------

void ABossMonster::SlamAttack()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (bSlamPending)
	{
		return; // a slam is already winding up — don't stack a second (overwrites center + orphans the disc)
	}

	// Freeze the impact zone at the boss's current position (it won't move during the wind-up because the
	// slam plays while the boss is in its attack cadence, but we freeze it anyway for correctness).
	SlamCenter = GetActorLocation();
	bSlamPending = true;
	SlamDetonateTime = World->GetTimeSeconds() + SlamWindupSeconds;

	// Spawn the telegraph disc at floor level so it paints the danger zone clearly.
	const float FloorZ = SlamCenter.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector TeleLoc(SlamCenter.X, SlamCenter.Y, FloorZ);

	FActorSpawnParameters TP;
	TP.Owner = this;
	TP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// AAttackTelegraph::Configure sets the lifespan to windup + 0.18s. Pass the full slam radius.
	SlamTelegraph = World->SpawnActor<AAttackTelegraph>(AAttackTelegraph::StaticClass(), FTransform(TeleLoc), TP);
	if (SlamTelegraph)
	{
		SlamTelegraph->Configure(SlamRadius, SlamWindupSeconds);
	}

	TriggerHitReact(); // wind-up tell

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 11, SlamWindupSeconds, FColor::Red,
			FString::Printf(TEXT("Boss: GROUND SLAM in %.1fs — JUMP!"), SlamWindupSeconds));
	}
}

// ---------------------------------------------------------------------------
// Reserved specials (tier 1+) — code preserved, not called at tier 0
// ---------------------------------------------------------------------------

void ABossMonster::SummonAdds()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FRandomStream Rng(FMath::Rand());
	for (int32 i = 0; i < SummonCount; ++i)
	{
		const FVector Offset(FMath::FRandRange(-300.f, 300.f), FMath::FRandRange(-300.f, 300.f), 100.f);
		if (AMonsterCharacter* Add = World->SpawnActor<AMonsterCharacter>(AMonsterCharacter::StaticClass(),
			FTransform(GetActorLocation() + Offset), Params))
		{
			Add->ApplyType(MonsterDatabase::RollRandomType(Rng)); // proper themed minion, not a blank base monster
		}
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
			Proj->Launch(Dir, ProjectileDamage, this, /*bTargetPlayer*/ true, /*GravityScale*/ 0.f, ProjectileSpeed);
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

void ABossMonster::EnterShellRetreat()
{
	bShellRetreat = true;
	ShellTimeLeft = FMath::Max(0.5f, ShellRetreatDuration);
	GetCharacterMovement()->StopMovementImmediately();
	TriggerHitReact();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 16, 1.5f, FColor::White, TEXT("Boss: RETREATS INTO ITS SHELL!"));
	}
}

void ABossMonster::UpdateShellRetreat(float DeltaSeconds)
{
	ShellTimeLeft = FMath::Max(0.f, ShellTimeLeft - DeltaSeconds);

	// Pull the body in (squashed + low) while shelled, easing back out as it re-emerges.
	if (BodyRoot)
	{
		const float A = (ShellRetreatDuration > 0.f) ? ShellTimeLeft / ShellRetreatDuration : 0.f; // 1 -> 0
		const float Squash = 0.7f + 0.3f * (1.f - A); // 0.7 while tucked, back to 1.0 as it opens
		BodyRoot->SetRelativeScale3D(FVector(BodyScale * Squash));
	}

	if (ShellTimeLeft <= 0.f)
	{
		bShellRetreat = false;
		if (BodyRoot) { BodyRoot->SetRelativeScale3D(FVector(BodyScale)); }
	}
}

// ---------------------------------------------------------------------------
// Phase advance (visual morph + stat buff)
// ---------------------------------------------------------------------------

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

	// Phases 2+ buff the boss (on top of the already tier-scaled stats). Correct for tier 0 (a single
	// 1->2 transition). TODO(tier1+): these multiply the LIVE values, so across 3-5 phases they COMPOUND
	// (e.g. AttackDamage *1.25 each step). When tier 1+ multi-phase content is designed, recompute the
	// per-phase buff from the tier-scaled base (e.g. BaseAttackDamage * TierScale * pow(1.25, NewPhase-1))
	// instead of multiplying in place, and stop overwriting BaseMoveSpeed here.
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

// ---------------------------------------------------------------------------
// Intro animation
// ---------------------------------------------------------------------------

void ABossMonster::PlayIntro()
{
	bIntroPlaying = true;

	GetCharacterMovement()->StopMovementImmediately();

	if (!SpawnAnim && !SpawnAnimPath.IsNull())
	{
		SpawnAnim = SpawnAnimPath.LoadSynchronous();
	}
	if (SpawnAnim && GetMesh() && GetMesh()->GetSkeletalMeshAsset())
	{
		GetMesh()->PlayAnimation(SpawnAnim, /*bLooping*/ false);
		IntroDuration = FMath::Max(0.1f, SpawnAnim->GetPlayLength());
	}
	else if (BodyRoot)
	{
		BodyRoot->SetRelativeScale3D(FVector(BodyScale * 0.2f));
	}

	IntroTimeLeft = FMath::Max(0.1f, IntroDuration);

	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters P;
		P.Owner = this;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const float CapHalf = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		const FVector FeetLoc = GetActorLocation() - FVector(0.f, 0.f, CapHalf);
		if (ABossSpawnVFX* VFX = World->SpawnActor<ABossSpawnVFX>(ABossSpawnVFX::StaticClass(), FTransform(FeetLoc), P))
		{
			VFX->Configure(IntroDuration, FLinearColor(1.0f, 0.45f, 0.12f));
		}
	}
}

void ABossMonster::UpdateIntro(float DeltaSeconds)
{
	IntroTimeLeft = FMath::Max(0.f, IntroTimeLeft - DeltaSeconds);
	const float A = 1.f - (IntroDuration > 0.f ? IntroTimeLeft / IntroDuration : 0.f);

	if (BodyRoot)
	{
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
		ScheduleNextSpecial();
	}
}

// ---------------------------------------------------------------------------
// Damage routing (weak point + shell invulnerability)
// ---------------------------------------------------------------------------

float ABossMonster::ApplyHitDamage(float BaseDamage, const FVector& FromLocation)
{
	// Invulnerable while shelled: hits clang off (visual react, no damage, no number).
	if (bShellRetreat)
	{
		TriggerHitReact();
		bLastHitWeak = false;
		return 0.f;
	}

	if (!Health)
	{
		return 0.f;
	}

	float Dmg = BaseDamage;
	bool bWeak = false;

	if (bBackWeakActive)
	{
		FVector ToHit = FromLocation - GetActorLocation();
		ToHit.Z = 0.f;
		if (ToHit.Normalize() && FVector::DotProduct(GetActorForwardVector(), ToHit) < -0.25f)
		{
			Dmg *= WeakPointMultiplier;
			bWeak = true;
		}
	}

	bLastHitWeak = bWeak;
	LastHitFromLocation = FromLocation;

	// Multi-life intercept: if this hit would kill the boss but lives remain, clamp the damage so HP
	// stops at 1 (the boss survives), then manually trigger the phase transition. This prevents the
	// base-class HandleDeath from firing for anything except the true final life.
	if (PhasesRemaining > 1 && Dmg >= Health->GetHealth())
	{
		// Clamp so we land exactly at 1 HP (visible to health bar, but not 0 so OnDepleted never fires).
		const float ClampedDmg = Health->GetHealth() - 1.f;
		const float Applied = (ClampedDmg > 0.f) ? Health->ApplyDamage(ClampedDmg) : 0.f;

		// Consume one life and advance.
		PhasesRemaining--;

		// Short dramatic pause: do the morph immediately (visual pop), then restore full HP.
		AdvanceToPhase(CurrentPhase + 1);

		// Refill HP for the new phase (also clears bDead on the health component, though it was never set).
		Health->SetMaxHealth(MonsterMaxHealth, /*bRefill*/ true);
		ScheduleNextSpecial();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(/*Key*/ 9, 3.f, FColor::Orange,
				FString::Printf(TEXT("Boss PHASE %d! Lives remaining: %d/%d"), CurrentPhase, PhasesRemaining, MaxPhases));
		}

		return Applied;
	}

	// Normal hit (or final-life killing blow — let it go through to 0 and fire OnDepleted/HandleDeath).
	return Health->ApplyDamage(Dmg);
}

// ---------------------------------------------------------------------------
// Line of sight check
// ---------------------------------------------------------------------------

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
	return !World->LineTraceTestByChannel(Start, End, ECC_Visibility, Params);
}

// ---------------------------------------------------------------------------
// Custom chase: crab scuttle / lunge
// ---------------------------------------------------------------------------

bool ABossMonster::TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist)
{
	// Always face the player so the back weak point and forward lunges stay meaningful.
	const FRotator Face(0.f, DirToPlayer.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 7.f));

	if (!HasLineOfSightToPlayer(Player))
	{
		bUsingDirectMove = false;
		return false;
	}
	if (!bUsingDirectMove)
	{
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		bUsingDirectMove = true;
		MoveState = EMoveState::Scuttle;
		MoveTimer = FMath::FRandRange(ScuttleTimeMin, ScuttleTimeMax);
	}

	const FVector Lateral = FVector::CrossProduct(FVector::UpVector, DirToPlayer).GetSafeNormal();
	MoveTimer -= DeltaSeconds;

	switch (MoveState)
	{
	case EMoveState::Scuttle:
	{
		const float DistErr = Dist - PreferredScuttleDist;
		const float Radial = FMath::Clamp(DistErr / 300.f, -1.f, 1.f);
		const FVector Move = (Lateral * StrafeSign + DirToPlayer * (0.6f * Radial)).GetSafeNormal();
		AddMovementInput(Move, 1.f);
		if (MoveTimer <= 0.f)
		{
			MoveState = EMoveState::Telegraph;
			MoveTimer = LungeTelegraph;
			TriggerHitReact();
		}
		else if (FMath::FRand() < DeltaSeconds * 0.6f)
		{
			StrafeSign *= -1.f;
		}
		break;
	}
	case EMoveState::Telegraph:
		if (MoveTimer <= 0.f)
		{
			MoveState = EMoveState::Lunge;
			MoveTimer = LungeTime;
			GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * LungeSpeedMult;
		}
		break;
	case EMoveState::Lunge:
		AddMovementInput(DirToPlayer, 1.f);
		if (MoveTimer <= 0.f)
		{
			MoveState = EMoveState::Retreat;
			MoveTimer = RetreatTime;
			GetCharacterMovement()->MaxWalkSpeed = (EnrageEndTime > 0.f) ? BaseMoveSpeed * 2.f : BaseMoveSpeed;
		}
		break;
	case EMoveState::Retreat:
		AddMovementInput((-DirToPlayer * 0.85f + Lateral * StrafeSign * 0.5f).GetSafeNormal(), 1.f);
		if (MoveTimer <= 0.f)
		{
			MoveState = EMoveState::Scuttle;
			MoveTimer = FMath::FRandRange(ScuttleTimeMin, ScuttleTimeMax);
			StrafeSign = (FMath::FRand() < 0.5f) ? -1.f : 1.f;
		}
		break;
	}

	return true;
}
