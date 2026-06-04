#include "MonsterCharacter.h"
#include "HealthComponent.h"
#include "StatsComponent.h"
#include "MonsterTypes.h"
#include "DeathPoof.h"
#include "DamageNumber.h"

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
	USkeletalMesh* Skel = !Def.SkeletalMeshPath.IsEmpty()
		? Cast<USkeletalMesh>(FSoftObjectPath(Def.SkeletalMeshPath).TryLoad()) : nullptr;

	if (Skel && GetMesh())
	{
		bUsingSkeletalBody = true;
		const float S = FMath::Max(0.05f, Def.MeshScale);
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

		RunAnim = Cast<UAnimSequence>(FSoftObjectPath(Def.RunAnimPath).TryLoad());
		IdleAnim = Cast<UAnimSequence>(FSoftObjectPath(Def.IdleAnimPath).TryLoad());
		AttackAnim = Cast<UAnimSequence>(FSoftObjectPath(Def.AttackAnimPath).TryLoad());
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		AnimState = ESkelAnim::None;
		SetLocomotion(/*bMoving*/ false); // start idle
	}
	else
	{
		bUsingSkeletalBody = false;
		if (BodyRoot) { BodyRoot->SetRelativeScale3D(FVector(BodyScale)); }
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

	if (Dist > AttackRange)
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
					AI->MoveToActor(Player, AttackRange * 0.85f, /*bStopOnOverlap*/ true, /*bUsePathfinding*/ true);
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
		if (UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>())
		{
			PlayerHealth->ApplyDamage(AttackDamage);
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
	if (!bUsingSkeletalBody || !GetMesh() || !AttackAnim)
	{
		return;
	}
	GetMesh()->PlayAnimation(AttackAnim, /*bLooping*/ false);
	AnimState = ESkelAnim::Attack;
	AttackAnimEndTime = GetWorld()->GetTimeSeconds() + AttackAnim->GetPlayLength();
}

float AMonsterCharacter::ApplyHitDamage(float BaseDamage, const FVector& FromLocation)
{
	// Base monsters have no weak points; clear the flag and apply the damage straight through.
	bLastHitWeak = false;
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

	// Floating damage number above the monster.
	if (Amount > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector Loc = GetActorLocation() + FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() + 35.f);
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ADamageNumber* Num = World->SpawnActor<ADamageNumber>(ADamageNumber::StaticClass(), FTransform(Loc), P))
			{
				Num->Init(Amount, bLastHitWeak);
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

	// Pop upward with gravity (launch then fall) and a fast tumble.
	const float Up = 320.f * Alpha - 700.f * Alpha * Alpha; // simple v0*t - 0.5*g*t^2 shape (cm)
	const FQuat Spin(DeathSpinAxis, FMath::DegreesToRadians(Alpha * 1080.f)); // up to ~3 turns about the axis
	DeathComp->SetRelativeLocation(DeathBaseLoc + FVector(0.f, 0.f, FMath::Max(0.f, Up)));
	DeathComp->SetRelativeRotation(Spin.Rotator());

	// Aggressive shrink to nothing.
	DeathComp->SetRelativeScale3D(DeathBaseScale * FMath::Max(0.f, 1.f - Alpha));

	if (DeathTimeLeft <= 0.f)
	{
		Destroy();
	}
}
