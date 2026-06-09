#include "WispMonster.h"
#include "HealthComponent.h"

#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AWispMonster::AWispMonster()
{
	// Flying — no gravity, no ground-walking.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GravityScale = 0.f;
	Move->DefaultLandMovementMode = MOVE_Flying;
	Move->bOrientRotationToMovement = false;

	// Aura range handles damage; suppress the base-class melee swing entirely.
	// Reach = AttackRange + max(0, capsuleRadius - 40); with 0 the wisp never enters attack mode.
	AttackRange = 0.f;

	// Point light — floats with the mesh, casts no shadow (performance).
	WispLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WispLight"));
	WispLight->SetupAttachment(GetMesh());
	WispLight->SetAttenuationRadius(300.f);
	WispLight->SetCastShadows(false);
	WispLight->SetIntensity(LightBaseIntensity);
	WispLight->SetLightColor(WispColor);
}

void AWispMonster::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (WispLight)
	{
		WispLight->SetLightColor(WispColor);
		WispLight->SetIntensity(LightBaseIntensity);
	}
}

void AWispMonster::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	// Apply editor-set color/intensity in case OnConstruction ran before BeginPlay changed anything.
	if (WispLight)
	{
		WispLight->SetLightColor(WispColor);
		WispLight->SetIntensity(LightBaseIntensity);
	}
}

void AWispMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// ---- Light flicker: Sin(Time × 4) → [0.8, 1.2] × base intensity ----
	if (WispLight)
	{
		const float T     = GetWorld()->GetTimeSeconds();
		const float Scale = FMath::GetMappedRangeValueClamped(
			FVector2D(-1.f, 1.f), FVector2D(0.8f, 1.2f),
			FMath::Sin(T * 4.f));
		WispLight->SetIntensity(LightBaseIntensity * Scale);
	}

	// ---- Proximity aura damage ----
	AuraTimer -= DeltaSeconds;
	if (AuraTimer > 0.f)
	{
		return;
	}
	AuraTimer = AuraDamageInterval;

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	if (FVector::Dist(GetActorLocation(), Player->GetActorLocation()) <= AuraRadius)
	{
		if (UHealthComponent* HP = Player->FindComponentByClass<UHealthComponent>())
		{
			HP->ApplyDamage(AuraDamage);
		}
	}
}

// ---------------------------------------------------------------------------
// Attack override — no melee; damage is dealt via aura in Tick
// ---------------------------------------------------------------------------

void AWispMonster::PlayAttackAnim()
{
	// Intentional no-op. The Wisp never swings; AuraDamage in Tick handles proximity damage.
}

// ---------------------------------------------------------------------------
// Movement — hover-drift toward the player
// ---------------------------------------------------------------------------

bool AWispMonster::TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return false;
	}

	// Hover target: player's XY position, HoverHeight above their origin.
	const FVector PlayerLoc = Player->GetActorLocation();
	const FVector Target(PlayerLoc.X, PlayerLoc.Y, PlayerLoc.Z + HoverHeight);

	// Drift toward target at DriftSpeed; coast to a stop when very close.
	const FVector Delta    = Target - GetActorLocation();
	const float   DistToTarget = Delta.Size();
	Move->Velocity = (DistToTarget > 8.f)
		? Delta.GetSafeNormal() * DriftSpeed
		: FVector::ZeroVector;

	// Gently face the player (yaw only).
	const FRotator FacePlayer(0.f, DirToPlayer.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), FacePlayer, DeltaSeconds, 3.f));

	return true;
}
