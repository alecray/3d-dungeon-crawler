#include "Projectile.h"
#include "MonsterCharacter.h"
#include "HealthComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/SoftObjectPath.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // only does work once the fireball visual (Tier 0 flicker) is on

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(10.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true); // generate Hit events
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.16f));
	if (UStaticMesh* Sphere = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")).TryLoad()))
	{
		Mesh->SetStaticMesh(Sphere);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.f; // straight flight

	// Fireball bits — created always, but inert until EnableFireballVisual() turns them on.
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(Collision);
	FireLight->SetCastShadows(false);
	FireLight->SetIntensity(0.f); // off for plain bolts

	FireFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireFX"));
	FireFX->SetupAttachment(Collision);
	FireFX->SetAutoActivate(false); // no asset / not a fireball yet

	// Tier 1 (auto-upgrade): pick up the authored systems if/when they exist. Null is fine — we fall
	// back to the Tier 0 orb. Use LoadObject so a missing asset just logs nothing and returns null.
	FireballSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Fireball.NS_Fireball"));
	ImpactFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_FireballImpact.NS_FireballImpact"));
}

void AProjectile::EnableFireballVisual()
{
	bFireball = true;

	if (FireballSystem)
	{
		// Tier 1: the authored Niagara system carries the whole look; hide the graybox orb.
		bUsingNiagara = true;
		FireFX->SetAsset(FireballSystem);
		FireFX->Activate(true);
		if (Mesh) { Mesh->SetVisibility(false); }
	}
	else if (Mesh)
	{
		// Tier 0: code-driven emissive orb. Bright orange "Color" (same recipe as ImpactBurst/Bonfire),
		// scaled up a touch so the bolt reads as a small fireball rather than a pellet.
		Mesh->SetRelativeScale3D(FVector(0.28f));
		if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			OrbMID = MID;
			OrbMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(6.f, 1.6f, 0.25f)); // HDR orange
		}
	}

	// The point light glows for both tiers (Niagara can light too, but this is cheap and consistent).
	if (FireLight)
	{
		FireLight->SetLightColor(FLinearColor(1.f, 0.45f, 0.12f));
		BaseLightIntensity = 4000.f;
		FireLight->SetIntensity(BaseLightIntensity);
		FireLight->SetAttenuationRadius(550.f);
	}
}

void AProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFireball || !FireLight)
	{
		return; // plain bolts do nothing here
	}

	// Cheap fire flicker: jitter the light intensity around its base each frame.
	FireElapsed += DeltaSeconds;
	const float Flicker = 0.82f + 0.18f * FMath::Sin(FireElapsed * 38.f) + FMath::FRandRange(-0.06f, 0.06f);
	FireLight->SetIntensity(BaseLightIntensity * Flicker);
}

void AProjectile::Launch(const FVector& Direction, float InDamage, AActor* Shooter, bool bTargetPlayer, float GravityScale, float InSpeed)
{
	Damage = InDamage;
	bDamagesPlayer = bTargetPlayer;

	if (Shooter)
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	Collision->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

	const float UseSpeed = (InSpeed >= 0.f) ? InSpeed : Speed;
	Movement->ProjectileGravityScale = GravityScale;
	Movement->Velocity = Direction.GetSafeNormal() * UseSpeed;
	SetLifeSpan(LifeSeconds);
}

void AProjectile::OnHit(UPrimitiveComponent* /*HitComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/,
	FVector /*NormalImpulse*/, const FHitResult& /*Hit*/)
{
	// Boss bolts hurt the player (anything with health that isn't a monster); player bolts hurt monsters.
	const bool bIsMonster = OtherActor && OtherActor->IsA(AMonsterCharacter::StaticClass());
	const bool bValidTarget = OtherActor && (bDamagesPlayer ? !bIsMonster : bIsMonster);
	if (bValidTarget)
	{
		if (!bDamagesPlayer)
		{
			// Player's bolt: route through the monster so back/weak-point hits are detected.
			if (AMonsterCharacter* Monster = Cast<AMonsterCharacter>(OtherActor))
			{
				Monster->ApplyHitDamage(Damage, GetActorLocation());
			}
		}
		else if (UHealthComponent* TargetHealth = OtherActor->FindComponentByClass<UHealthComponent>())
		{
			TargetHealth->ApplyDamage(Damage);
		}
	}

	// Fireball impact burst (Tier 1 art; no-op until NS_FireballImpact is authored).
	if (bFireball && ImpactFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, GetActorLocation(), GetActorRotation());
	}

	Destroy();
}
