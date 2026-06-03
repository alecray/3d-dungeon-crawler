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

void AProjectile::Launch(const FVector& Direction, float InDamage, AActor* Shooter)
{
	Damage = InDamage;

	if (Shooter)
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	Collision->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

	Movement->Velocity = Direction.GetSafeNormal() * Speed;
	SetLifeSpan(LifeSeconds);
}

void AProjectile::OnHit(UPrimitiveComponent* /*HitComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/,
	FVector /*NormalImpulse*/, const FHitResult& /*Hit*/)
{
	if (OtherActor && OtherActor->IsA(AMonsterCharacter::StaticClass()))
	{
		if (UHealthComponent* MonsterHealth = OtherActor->FindComponentByClass<UHealthComponent>())
		{
			MonsterHealth->ApplyDamage(Damage);
		}
	}
	Destroy();
}
