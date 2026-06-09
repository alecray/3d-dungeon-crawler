#include "AmbientDust.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/SoftObjectPath.h"

AAmbientDust::AAmbientDust()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

FVector AAmbientDust::PlayerLoc() const
{
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		return Cam->GetCameraLocation();
	}
	return GetActorLocation();
}

FVector AAmbientDust::RandomSpotNear(const FVector& Center) const
{
	return Center + FVector(
		FMath::FRandRange(-Radius, Radius),
		FMath::FRandRange(-Radius, Radius),
		FMath::FRandRange(-Radius * 0.35f, Radius * 0.6f));
}

void AAmbientDust::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cube = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());
	const FVector Center = PlayerLoc();

	for (int32 i = 0; i < MoteCount; ++i)
	{
		UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
		M->SetupAttachment(GetRootComponent());
		M->RegisterComponent();
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		if (Cube)
		{
			M->SetStaticMesh(Cube);
			if (UMaterialInstanceDynamic* MID = M->CreateAndSetMaterialInstanceDynamic(0))
			{
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.5f, 0.42f));
			}
		}
		M->SetWorldScale3D(FVector(MoteScale));
		M->SetWorldLocation(RandomSpotNear(Center));

		Velocities.Add(FMath::VRand() * DriftSpeed * FMath::FRandRange(0.5f, 1.f));
		Motes.Add(M);
	}
}

void AAmbientDust::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector Center = PlayerLoc();
	const float RadiusSq = Radius * Radius;

	for (int32 i = 0; i < Motes.Num(); ++i)
	{
		UStaticMeshComponent* M = Motes[i];
		if (!M)
		{
			continue;
		}
		FVector Loc = M->GetComponentLocation() + Velocities[i] * DeltaSeconds;

		// Recycle any mote that drifts out of range to a fresh spot near the player (constant cost).
		if (FVector::DistSquared(Loc, Center) > RadiusSq)
		{
			Loc = RandomSpotNear(Center);
			Velocities[i] = FMath::VRand() * DriftSpeed * FMath::FRandRange(0.5f, 1.f);
		}
		M->SetWorldLocation(Loc);
	}
}
