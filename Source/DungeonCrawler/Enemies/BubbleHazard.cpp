#include "BubbleHazard.h"
#include "HealthComponent.h"
#include "MonsterCharacter.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/SoftObjectPath.h"

// The engine sphere is 50cm radius, so a component scale of 1.0 == 50cm radius.
static constexpr float SphereUnitRadius = 50.f;

ABubbleHazard::ABubbleHazard()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(Radius);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(Trigger);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* Sphere = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")).TryLoad()))
	{
		Mesh->SetStaticMesh(Sphere);
	}
}

void ABubbleHazard::Init(float InRadius, float InDamage, float InLifetime)
{
	Radius = FMath::Max(20.f, InRadius);
	Damage = InDamage;
	Lifetime = FMath::Max(0.5f, InLifetime);

	if (Trigger) { Trigger->SetSphereRadius(Radius); }
	if (Mesh) { Mesh->SetRelativeScale3D(FVector(Radius / SphereUnitRadius)); }
}

void ABubbleHazard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;

	// Grow-in over the first 0.3s, then a quick pop-out shrink over the last 0.3s.
	float ScaleMul = 1.f;
	if (Age < 0.3f) { ScaleMul = Age / 0.3f; }
	else if (Age > Lifetime - 0.3f) { ScaleMul = FMath::Max(0.f, (Lifetime - Age) / 0.3f); }
	if (Mesh) { Mesh->SetRelativeScale3D(FVector((Radius / SphereUnitRadius) * ScaleMul)); }

	// Only deal damage once it has finished growing in.
	if (Age >= 0.3f)
	{
		DamageTimer -= DeltaSeconds;
		if (DamageTimer <= 0.f)
		{
			DamageInside();
			DamageTimer = DamageInterval;
		}
	}

	if (Age >= Lifetime)
	{
		Destroy();
	}
}

void ABubbleHazard::DamageInside()
{
	if (!Trigger)
	{
		return;
	}

	TArray<AActor*> Overlap;
	Trigger->GetOverlappingActors(Overlap, APawn::StaticClass());
	for (AActor* Actor : Overlap)
	{
		// Bubbles are a boss attack: hurt the player, never other monsters/the boss.
		if (!Actor || Actor->IsA(AMonsterCharacter::StaticClass()))
		{
			continue;
		}
		if (UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>())
		{
			if (!Health->IsDead())
			{
				Health->ApplyDamage(Damage);
			}
		}
	}
}
