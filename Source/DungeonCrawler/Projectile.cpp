#include "Projectile.h"
#include "MonsterCharacter.h"
#include "HealthComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/SoftObjectPath.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

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
}

void AProjectile::Launch(const FVector& Direction, float InDamage, AActor* Shooter, bool bTargetPlayer, float GravityScale)
{
	Damage = InDamage;
	bDamagesPlayer = bTargetPlayer;

	if (Shooter)
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	Collision->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

	Movement->ProjectileGravityScale = GravityScale;
	Movement->Velocity = Direction.GetSafeNormal() * Speed;
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
	Destroy();
}
