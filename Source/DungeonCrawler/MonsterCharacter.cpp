#include "MonsterCharacter.h"
#include "HealthComponent.h"
#include "StatsComponent.h"
#include "MonsterTypes.h"
#include "DeathPoof.h"
#include "DamageNumber.h"
#include "ImpactBurst.h"
#include "AttackTelegraph.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h" // EPathFollowingRequestResult values
#include "NavigationInvokerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Misc/FrameRate.h"
#include "Engine/Engine.h" // GEngine on-screen debug

// The engine cube is a 100cm cube centered on its origin, so a component scale of 1.0 == 100cm.
static constexpr float MonsterUnitCm = 100.f;

AMonsterCharacter::AMonsterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 88.f);

	// Steer by facing the movement direction; the AIController only feeds movement input.
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
	Movement->MaxWalkSpeed = MoveSpeed;

	// Auto-possess so AddMovementInput / path-following is consumed even though no player drives it.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	// Build a navmesh tile around the monster at runtime (no baked navmesh in the procedural dungeon),
	// so the AIController's MoveTo can path around walls.
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(3500.f, 5000.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}

	// Inherited skeletal mesh (ACharacter::GetMesh) is used only for skeletal monster types (e.g. the
	// crab); hidden by default in favor of the graybox cube body.
	if (USkeletalMeshComponent* SkelBody = GetMesh())
	{
		SkelBody->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
		SkelBody->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		SkelBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkelBody->SetVisibility(false);
	}

	// All body meshes hang off BodyRoot so the hit-react can scale the whole monster at once.
	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(GetCapsuleComponent());

	// Hunched humanoid graybox: torso, head, two dangling arms.
	BodyMesh = AddBox(TEXT("BodyMesh"), FVector(0.f, 0.f, 10.f), FVector(55.f, 40.f, 90.f));
	HeadMesh = AddBox(TEXT("HeadMesh"), FVector(10.f, 0.f, 70.f), FVector(40.f, 38.f, 40.f));
	LeftArmMesh = AddBox(TEXT("LeftArmMesh"), FVector(15.f, -36.f, 0.f), FVector(18.f, 18.f, 80.f));
	RightArmMesh = AddBox(TEXT("RightArmMesh"), FVector(15.f, 36.f, 0.f), FVector(18.f, 18.f, 80.f));
}

UStaticMeshComponent* AMonsterCharacter::AddBox(const TCHAR* Name, const FVector& Center, const FVector& SizeCm)
{
	UStaticMeshComponent* Box = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Box->SetupAttachment(BodyRoot);
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision); // the capsule handles collision
	Box->SetRelativeLocation(Center);
	Box->SetRelativeScale3D(SizeCm / MonsterUnitCm);
	if (CubeMesh)
	{
		Box->SetStaticMesh(CubeMesh);
	}
	return Box;
}

void AMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	if (BodyRoot)
	{
		BodyRoot->SetRelativeScale3D(FVector(BodyScale));
	}

	if (Health)
	{
		Health->SetMaxHealth(MonsterMaxHealth, /*bRefill*/ true);
		Health->OnDepleted.AddUObject(this, &AMonsterCharacter::HandleDeath);
		Health->OnDamaged.AddUObject(this, &AMonsterCharacter::HandleDamaged);
	}
}

void AMonsterCharacter::ApplyType(FName TypeId)
{
	if (!MonsterDatabase::Contains(TypeId))
	{
		return;
	}
	const FMonsterDef& Def = MonsterDatabase::Get(TypeId);

	// Stats.
	MonsterMaxHealth = Def.MaxHealth;
	MoveSpeed = Def.MoveSpeed;
	AttackDamage = Def.AttackDamage;
	AttackRange = Def.AttackRange;
	XPReward = Def.XPReward;
	BodyScale = Def.BodyScale;

	GetCapsuleComponent()->SetCapsuleSize(Def.CapsuleRadius, Def.CapsuleHalfHeight);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	if (Health)
	{
		Health->SetMaxHealth(MonsterMaxHealth, /*bRefill*/ true);
	}

	// Skeletal body if a mesh is available; otherwise keep the graybox cube body.
	if (!SetupSkeletalBody(Def.SkeletalMeshPath, Def.MeshScale, Def.RunAnimPath, Def.IdleAnimPath, Def.AttackAnimPath))
	{
		bUsingSkeletalBody = false;
		if (BodyRoot) { BodyRoot->SetRelativeScale3D(FVector(BodyScale)); }
	}
}

bool AMonsterCharacter::SetupSkeletalBody(const FString& MeshPath, float MeshScale,
	const FString& RunPath, const FString& IdlePath, const FString& AttackPath)
{
	USkeletalMesh* Skel = !MeshPath.IsEmpty() ? Cast<USkeletalMesh>(FSoftObjectPath(MeshPath).TryLoad()) : nullptr;
	if (!Skel || !GetMesh())
	{
		return false;
	}

	bUsingSkeletalBody = true;
	const float S = FMath::Max(0.05f, MeshScale);
	GetMesh()->SetSkeletalMesh(Skel);
	GetMesh()->SetRelativeScale3D(FVector(S));
	GetMesh()->SetVisibility(true);
	if (BodyRoot) { BodyRoot->SetVisibility(true, /*bPropagate*/ true); BodyRoot->SetHiddenInGame(true, true); }

	// Auto-fit the collision capsule (hitbox) to the scaled mesh bounds so they always match.
	const FBoxSphereBounds B = Skel->GetBounds();
	const float Radius = FMath::Max(8.f, FMath::Max(B.BoxExtent.X, B.BoxExtent.Y) * S);
	const float HalfHeight = FMath::Max(Radius, B.BoxExtent.Z * S);
	GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);

	// Drop the mesh so the bottom of its bounds sits at the capsule's base.
	const float MeshBottomZ = (B.Origin.Z - B.BoxExtent.Z) * S;
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight - MeshBottomZ));

	// Load each clip, then flag any that didn't load or are bound to a different skeleton than this mesh
	// (a mismatched/empty clip plays nothing via PlayAnimation — the usual reason an anim "doesn't show").
	USkeleton* MeshSkel = Skel->GetSkeleton();
	auto LoadAnim = [&](const FString& Path, const TCHAR* Label) -> UAnimSequence*
	{
		if (Path.IsEmpty()) { return nullptr; }
		UAnimSequence* Anim = Cast<UAnimSequence>(FSoftObjectPath(Path).TryLoad());
		if (!Anim)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Monster %s] %s anim failed to load (missing or not an AnimSequence): %s"),
				*GetName(), Label, *Path);
		}
		else if (MeshSkel && Anim->GetSkeleton() != MeshSkel)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Monster %s] %s anim '%s' is bound to a DIFFERENT skeleton (%s) than the mesh (%s) — it will not play. Reimport it onto %s."),
				*GetName(), Label, *Anim->GetName(),
				*GetNameSafe(Anim->GetSkeleton()), *GetNameSafe(MeshSkel), *GetNameSafe(MeshSkel));
		}
		return Anim;
	};
	RunAnim = LoadAnim(RunPath, TEXT("Run/Walk"));
	IdleAnim = LoadAnim(IdlePath, TEXT("Idle"));
	AttackAnim = LoadAnim(AttackPath, TEXT("Attack"));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	AnimState = ESkelAnim::None;
	SetLocomotion(/*bMoving*/ false); // start idle
	return true;
}

void AMonsterCharacter::MakeElite()
{
	MonsterMaxHealth *= 4.f;
	AttackDamage *= 1.6f;
	XPReward *= 4;
	BodyScale *= 1.35f;

	if (Health)
	{
		Health->SetMaxHealth(MonsterMaxHealth, /*bRefill*/ true);
	}
	// Enlarge the visible body (graybox uses BodyRoot; skeletal types use the inherited mesh).
	if (bUsingSkeletalBody && GetMesh())
	{
		GetMesh()->SetRelativeScale3D(GetMesh()->GetRelativeScale3D() * 1.35f);
	}
	else if (BodyRoot)
	{
		BodyRoot->SetRelativeScale3D(FVector(BodyScale));
	}
}

void AMonsterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Hit-react pop (runs even while dying so the killing blow still reacts): the body scales up on
	// impact and eases back to normal.
	if (HitReactTimeLeft > 0.f && BodyRoot && HitReactDuration > 0.f)
	{
		HitReactTimeLeft = FMath::Max(0.f, HitReactTimeLeft - DeltaSeconds);
		const float A = HitReactTimeLeft / HitReactDuration;     // 1 right after the hit -> 0
		const float Pop = A * HitReactScale;                      // snap to peak on impact, ease back
		BodyRoot->SetRelativeScale3D(FVector(BodyScale * (1.f + Pop)));
		if (HitReactTimeLeft <= 0.f)
		{
			BodyRoot->SetRelativeScale3D(FVector(BodyScale));
		}
	}

	if (bDead)
	{
		UpdateDeathEffect(DeltaSeconds);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bAttacking = (Now < AttackAnimEndTime); // attack anim in progress: hold it

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		if (!bAttacking) { SetLocomotion(false); }
		return;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	const float Dist = ToPlayer.Size();
	if (Dist > AggroRange || Dist < KINDA_SMALL_NUMBER)
	{
		if (!bAttacking) { SetLocomotion(false); }
		return;
	}

	const FVector Dir = ToPlayer / Dist;
	AAIController* AI = Cast<AAIController>(GetController());

	// Melee reach is measured from the body's edge, not its centre: a big body (the boss's auto-fit
	// capsule) can't let the player's centre get within a small AttackRange because the capsules collide
	// first. Extending the reach by the radius beyond the 40cm baseline lets the front claw connect from
	// "in front of the body" while leaving normal (~40cm-radius) monsters exactly as tuned.
	const float Reach = AttackRange + FMath::Max(0.f, GetCapsuleComponent()->GetScaledCapsuleRadius() - 40.f);

	// Resolve a pending (animation-timed) melee hit: the damage lands mid-swing on the hit frame, not when
	// the swing starts, so the player can dodge by leaving reach before then. Runs regardless of the
	// current range branch so a player who stepped out still whiffs the blow.
	if (bHitPending && Now >= PendingHitTime)
	{
		bHitPending = false;
		if (Dist <= Reach) // still in front of the claw at the moment of impact -> it connects
		{
			if (UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>())
			{
				PlayerHealth->ApplyDamage(PendingDamage);
			}
		}
	}

	if (Dist > Reach)
	{
		if (!bAttacking) { SetLocomotion(true); }

		// Subclasses (the boss) can take over movement here; otherwise use the default navmesh chase.
		if (!TickCustomChase(DeltaSeconds, Player, Dir, Dist))
		{
			// Chase via navmesh so we path around walls. Re-issue periodically since the player moves.
			RepathTimer -= DeltaSeconds;
			if (AI && RepathTimer <= 0.f)
			{
				RepathTimer = RepathInterval;
				const EPathFollowingRequestResult::Type Result =
					AI->MoveToActor(Player, Reach * 0.85f, /*bStopOnOverlap*/ true, /*bUsePathfinding*/ true);
				bNavChasing = (Result != EPathFollowingRequestResult::Failed); // navmesh tile ready?
			}
			if (!bNavChasing)
			{
				AddMovementInput(Dir, 1.f); // fallback: steer straight until the navmesh has generated
			}
		}
		return;
	}

	// In range: stop pathing, face the player, and swing on cooldown.
	if (AI) { AI->StopMovement(); }
	bNavChasing = false;
	const FRotator Desired = Dir.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Desired.Yaw, 0.f), DeltaSeconds, 8.f));

	if (Now - LastAttackTime >= AttackCooldown)
	{
		LastAttackTime = Now;
		PlayAttackAnim();

		// If a hit frame is configured and we have the clip, the damage lands later (mid-swing, dodgeable);
		// otherwise it lands immediately as before.
		float HitDelay = 0.f;
		if (AttackHitFrame >= 0.f && bUsingSkeletalBody && AttackAnim)
		{
			const double Fps = AttackAnim->GetSamplingFrameRate().AsDecimal();
			const float SafeFps = (Fps > 1.0) ? (float)Fps : 30.f;
			HitDelay = FMath::Clamp(AttackHitFrame / SafeFps, 0.f, AttackAnim->GetPlayLength());
		}

		if (HitDelay > 0.f)
		{
			PendingDamage = AttackDamage;
			PendingHitTime = Now + HitDelay;
			bHitPending = true;

			// Paint the danger zone on the floor: a red disc of the exact hit radius (Reach), centered on
			// us, that flashes when the blow lands — dash out of it to dodge.
			if (UWorld* World = GetWorld())
			{
				const float FloorZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				const FVector TeleLoc(GetActorLocation().X, GetActorLocation().Y, FloorZ);
				FActorSpawnParameters TP;
				TP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (AAttackTelegraph* Tele = World->SpawnActor<AAttackTelegraph>(AAttackTelegraph::StaticClass(), FTransform(TeleLoc), TP))
				{
					Tele->Configure(Reach, HitDelay);
				}
			}
		}
		else if (UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>())
		{
			PlayerHealth->ApplyDamage(AttackDamage); // immediate (no anim / no hit frame configured)
		}
	}
	else if (!bAttacking)
	{
		SetLocomotion(false); // idle between swings
	}
}

void AMonsterCharacter::SetLocomotion(bool bMoving)
{
	if (!bUsingSkeletalBody || !GetMesh())
	{
		return;
	}
	const ESkelAnim Desired = bMoving ? ESkelAnim::Run : ESkelAnim::Idle;
	if (AnimState == Desired)
	{
		return;
	}
	UAnimSequence* Anim = bMoving ? RunAnim.Get() : IdleAnim.Get();
	if (!Anim) { Anim = bMoving ? IdleAnim.Get() : RunAnim.Get(); } // fall back if one is missing
	if (Anim)
	{
		GetMesh()->PlayAnimation(Anim, /*bLooping*/ true);
		AnimState = Desired;
	}
}

void AMonsterCharacter::PlayAttackAnim()
{
	if (!bUsingSkeletalBody || !GetMesh())
	{
		return;
	}
	if (!AttackAnim)
	{
		// The swing still lands (damage already applied) but there's no clip to show.
		if (bLogAttackAnim && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
				FString::Printf(TEXT("%s: melee fired but AttackAnim is null (see Output Log)"), *GetName()));
		}
		return;
	}
	GetMesh()->PlayAnimation(AttackAnim, /*bLooping*/ false);
	AnimState = ESkelAnim::Attack;
	AttackAnimEndTime = GetWorld()->GetTimeSeconds() + AttackAnim->GetPlayLength();
	if (bLogAttackAnim && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("%s: attack anim '%s' (%.2fs)"), *GetName(), *AttackAnim->GetName(), AttackAnim->GetPlayLength()));
	}
}

float AMonsterCharacter::ApplyHitDamage(float BaseDamage, const FVector& FromLocation)
{
	// Backstab: a strike landing from BEHIND the monster deals bonus damage. Flagged like a weak-point hit
	// so it shows the bigger orange number + spark, rewarding flanking (Dark-Souls style).
	bLastHitWeak = false;
	FVector ToAttacker = FromLocation - GetActorLocation();
	ToAttacker.Z = 0.f;
	if (ToAttacker.Normalize() && FVector::DotProduct(GetActorForwardVector(), ToAttacker) < -0.3f)
	{
		BaseDamage *= BackstabMultiplier;
		bLastHitWeak = true;
	}

	const float Dealt = Health ? Health->ApplyDamage(BaseDamage) : 0.f;

	// Juice: knock the monster back away from where the hit came from (small upward hop).
	if (Dealt > 0.f && !bDead && KnockbackStrength > 0.f)
	{
		FVector Dir = GetActorLocation() - FromLocation;
		Dir.Z = 0.f;
		if (Dir.Normalize())
		{
			LaunchCharacter(Dir * KnockbackStrength + FVector(0.f, 0.f, 120.f), /*bXYOverride*/ true, /*bZOverride*/ false);
		}
	}
	return Dealt;
}

void AMonsterCharacter::HandleDamaged(UHealthComponent* /*DamagedComponent*/, float Amount)
{
	HitReactTimeLeft = HitReactDuration; // kick off the pop

	// Floating damage number above the monster + a quick impact spark at the body.
	if (Amount > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			const FVector NumberLoc = GetActorLocation() + FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() + 60.f);
			if (ADamageNumber* Num = World->SpawnActor<ADamageNumber>(ADamageNumber::StaticClass(), FTransform(NumberLoc), P))
			{
				Num->Init(Amount, bLastHitWeak);
			}

			const FVector ImpactLoc = GetActorLocation() + FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() * 0.5f);
			// Dark-Souls-y hit spray: a small spurt of dark blood bits with almost no light flash (the old
			// bright warm flash read as cartoony). Deferred so the config + color land before BeginPlay.
			if (AImpactBurst* Burst = World->SpawnActorDeferred<AImpactBurst>(
				AImpactBurst::StaticClass(), FTransform(ImpactLoc), this, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				Burst->Configure(/*Count*/ 6, /*Speed*/ 320.f, /*BitScale*/ 0.06f, /*Life*/ 0.32f,
					/*FlashIntensity*/ bLastHitWeak ? 700.f : 220.f, /*FlashRadius*/ 160.f);
				Burst->SetColor(bLastHitWeak ? FLinearColor(0.7f, 0.18f, 0.04f) : FLinearColor(0.45f, 0.06f, 0.05f)); // dark blood
				UGameplayStatics::FinishSpawningActor(Burst, FTransform(ImpactLoc));
			}
		}
	}
	bLastHitWeak = false; // consume the flag so unrouted damage (traps, etc.) shows as a normal hit
}

void AMonsterCharacter::HandleDeath(UHealthComponent* /*DeadComponent*/)
{
	bDead = true;

	// Award XP to the player.
	if (XPReward > 0)
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (UStatsComponent* PlayerStats = Player->FindComponentByClass<UStatsComponent>())
			{
				PlayerStats->AddXP(XPReward);
			}
		}
	}

	// Stop chasing, drop collision, and let the corpse linger briefly before cleanup.
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);

	if (AController* C = GetController())
	{
		C->UnPossess();
	}

	// Poof of particles at death.
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<ADeathPoof>(ADeathPoof::StaticClass(),
			FTransform(GetActorLocation() + FVector(0.f, 0.f, 30.f)), P);
	}

	// Pop-up & launch death effect: animate the visible mesh (skeletal crab or graybox body) up with a
	// spin while shrinking to nothing over DeathDuration, then destroy. The poof covers the vanish.
	DeathComp = bUsingSkeletalBody ? Cast<USceneComponent>(GetMesh()) : Cast<USceneComponent>(BodyRoot);
	if (DeathComp)
	{
		DeathBaseScale = DeathComp->GetRelativeScale3D();
		DeathBaseLoc = DeathComp->GetRelativeLocation();
	}
	// Random spin axis (biased upward) so each death tumbles differently.
	DeathSpinAxis = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(0.5f, 1.5f)).GetSafeNormal();
	DeathTimeLeft = DeathDuration;

	SetLifeSpan(DeathDuration + 0.5f); // safety net in case Tick is disabled
}

void AMonsterCharacter::UpdateDeathEffect(float DeltaSeconds)
{
	if (DeathTimeLeft <= 0.f || !DeathComp)
	{
		return;
	}
	DeathTimeLeft = FMath::Max(0.f, DeathTimeLeft - DeltaSeconds);

	const float Alpha = 1.f - (DeathTimeLeft / DeathDuration); // 0 -> 1 over the effect

	// Dissolve: sink the body into the floor with a slow tumble while it shrinks away — reads as the
	// corpse disintegrating rather than a cartoon pop. The death poof covers the final vanish.
	const float SinkDepth = DeathBaseScale.Z > 0.f ? 120.f : 90.f;       // how far it sinks (cm)
	const float Sink = -SinkDepth * FMath::Pow(Alpha, 1.5f);             // accelerate downward
	const FQuat Spin(DeathSpinAxis, FMath::DegreesToRadians(Alpha * 240.f)); // gentle settle, not a tumble
	DeathComp->SetRelativeLocation(DeathBaseLoc + FVector(0.f, 0.f, Sink));
	DeathComp->SetRelativeRotation(Spin.Rotator());

	// Shrink to nothing as it sinks (ease in so it lingers a touch, then collapses).
	DeathComp->SetRelativeScale3D(DeathBaseScale * FMath::Max(0.f, 1.f - Alpha * Alpha));

	if (DeathTimeLeft <= 0.f)
	{
		Destroy();
	}
}
