#include "BossMonster.h"
#include "HealthComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	CurrentPhase = 0;
	AdvanceToPhase(1);
	ScheduleNextSpecial();
}

void ABossMonster::Tick(float DeltaSeconds)
{
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

void ABossMonster::AdvanceToPhase(int32 NewPhase)
{
	CurrentPhase = NewPhase;

	if (MorphParts.IsValidIndex(NewPhase - 1) && MorphParts[NewPhase - 1])
	{
		MorphParts[NewPhase - 1]->SetVisibility(true);
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
	TArray<int32, TInlineAllocator<3>> Pool;
	Pool.Add(0); // slam (always)
	if (CurrentPhase >= 2) { Pool.Add(1); } // summon
	if (CurrentPhase >= 3) { Pool.Add(2); } // enrage

	switch (Pool[FMath::RandRange(0, Pool.Num() - 1)])
	{
	case 0: SlamAttack(); break;
	case 1: SummonAdds(); break;
	case 2: EnrageBurst(); break;
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
