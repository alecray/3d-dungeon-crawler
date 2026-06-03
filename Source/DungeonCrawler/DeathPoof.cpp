#include "DeathPoof.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/SoftObjectPath.h"

ADeathPoof::ADeathPoof()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ADeathPoof::SetColor(const FLinearColor& Color)
{
	Tint = Color;
}

void ADeathPoof::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cube = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());

	for (int32 i = 0; i < Count; ++i)
	{
		UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
		P->SetupAttachment(GetRootComponent());
		P->RegisterComponent();
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		if (Cube)
		{
			P->SetStaticMesh(Cube);
			if (UMaterialInstanceDynamic* MID = P->CreateAndSetMaterialInstanceDynamic(0))
			{
				// BasicShapeMaterial may ignore this, but it's harmless if so.
				MID->SetVectorParameterValue(TEXT("Color"), Tint);
			}
		}
		P->SetRelativeScale3D(FVector(ParticleScale));

		// Random outward direction, slight upward bias.
		FVector Dir = FMath::VRand();
		Dir.Z = FMath::Abs(Dir.Z) * 0.6f + 0.2f;
		Dir = Dir.GetSafeNormal();
		P->SetRelativeLocation(Dir * 8.f);
		Velocities.Add(Dir * Speed * FMath::FRandRange(0.6f, 1.2f));
		Parts.Add(P);
	}

	SetLifeSpan(LifeSeconds);
}

void ADeathPoof::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float Alpha = (LifeSeconds > 0.f) ? FMath::Clamp(Elapsed / LifeSeconds, 0.f, 1.f) : 1.f;
	const float Scale = ParticleScale * (1.f - Alpha); // shrink to nothing

	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		if (UStaticMeshComponent* P = Parts[i])
		{
			P->AddRelativeLocation(Velocities[i] * DeltaSeconds);
			P->SetRelativeScale3D(FVector(FMath::Max(0.f, Scale)));
		}
	}
}
