#include "ImpactBurst.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/SoftObjectPath.h"

AImpactBurst::AImpactBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(GetRootComponent());
	Flash->SetCastShadows(false);
	Flash->SetIntensity(0.f);
}

void AImpactBurst::BeginPlay()
{
	Super::BeginPlay();

	if (Flash)
	{
		Flash->SetLightColor(Tint);
		Flash->SetIntensity(FlashIntensity);
		Flash->SetAttenuationRadius(FlashRadius);
	}

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
				MID->SetVectorParameterValue(TEXT("Color"), Tint);
			}
		}
		P->SetRelativeScale3D(FVector(BitScale));

		FVector Dir = FMath::VRand();
		Dir.Z = FMath::Abs(Dir.Z) * 0.6f + 0.25f; // bias outward + up
		Dir = Dir.GetSafeNormal();
		Velocities.Add(Dir * Speed * FMath::FRandRange(0.7f, 1.3f));
		Bits.Add(P);
	}

	SetLifeSpan(LifeSeconds);
}

void AImpactBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float Alpha = (LifeSeconds > 0.f) ? FMath::Clamp(Elapsed / LifeSeconds, 0.f, 1.f) : 1.f;

	// Bits fly out, decelerate, and shrink to nothing.
	for (int32 i = 0; i < Bits.Num(); ++i)
	{
		if (UStaticMeshComponent* P = Bits[i])
		{
			P->AddRelativeLocation(Velocities[i] * DeltaSeconds);
			Velocities[i] *= 1.f - 4.f * DeltaSeconds; // drag
			P->SetRelativeScale3D(FVector(BitScale * (1.f - Alpha)));
		}
	}

	// The flash blinks out fast (over the first third of the life).
	if (Flash)
	{
		const float FlashA = FMath::Clamp(Elapsed / (LifeSeconds * 0.35f), 0.f, 1.f);
		Flash->SetIntensity(FlashIntensity * (1.f - FlashA));
	}
}
