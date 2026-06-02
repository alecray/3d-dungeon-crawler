#include "MonsterCharacter.h"
#include "HealthComponent.h"
#include "StatsComponent.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// The engine cube is a 100cm cube centered on its origin, so a component scale of 1.0 == 100cm.
static constexpr float CubeUnitCm = 100.f;

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

	// Auto-possess so AddMovementInput is consumed even though no player drives it.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
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
	Box->SetRelativeScale3D(SizeCm / CubeUnitCm);
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
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	const float Dist = ToPlayer.Size();
	if (Dist > AggroRange || Dist < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Dir = ToPlayer / Dist;

	if (Dist > AttackRange)
	{
		// Chase: movement orientation turns us to face the player as we go.
		AddMovementInput(Dir, 1.f);
		return;
	}

	// In range: face the player and swing on cooldown.
	const FRotator Desired = Dir.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Desired.Yaw, 0.f), DeltaSeconds, 8.f));

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime >= AttackCooldown)
	{
		LastAttackTime = Now;
		if (UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>())
		{
			PlayerHealth->ApplyDamage(AttackDamage);
		}
	}
}

void AMonsterCharacter::HandleDamaged(UHealthComponent* /*DamagedComponent*/, float /*Amount*/)
{
	HitReactTimeLeft = HitReactDuration; // kick off the pop
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

	// Tip over so death reads clearly in graybox.
	AddActorLocalRotation(FRotator(80.f, 0.f, 0.f));

	if (AController* C = GetController())
	{
		C->UnPossess();
	}
	SetLifeSpan(4.f);
}
