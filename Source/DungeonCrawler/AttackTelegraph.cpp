#include "AttackTelegraph.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"

AAttackTelegraph::AAttackTelegraph()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(GetRootComponent());
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);
}

void AAttackTelegraph::Configure(float InRadius, float InWindupSeconds)
{
	Radius = FMath::Max(20.f, InRadius);
	Windup = FMath::Max(0.05f, InWindupSeconds);
	SetLifeSpan(Windup + 0.18f); // safety net past the impact flash
}

void AAttackTelegraph::BeginPlay()
{
	Super::BeginPlay();

	if (Disc)
	{
		if (UStaticMesh* Cyl = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad()))
		{
			Disc->SetStaticMesh(Cyl);
		}
		// Engine cylinder is 100cm tall / 50cm radius (pivot centered). Flatten to a thin floor disc and
		// scale its radius to the danger radius; lift a hair so it doesn't z-fight the floor.
		const float XY = Radius / 50.f;
		Disc->SetRelativeScale3D(FVector(XY, XY, 0.02f));
		Disc->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
		if (UMaterialInstanceDynamic* MID = Disc->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.5f, 0.f, 0.f));
		}
	}
}

void AAttackTelegraph::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float A = FMath::Clamp(Elapsed / Windup, 0.f, 1.f); // 0 -> 1 over the wind-up

	if (Disc)
	{
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Disc->GetMaterial(0)))
		{
			// Brighten from dull to vivid red as the blow nears; a hard white-hot flash at impact.
			const float Pulse = 0.6f + 0.4f * FMath::Sin(Elapsed * 22.f);
			FLinearColor Col = FLinearColor::LerpUsingHSV(FLinearColor(0.4f, 0.f, 0.f), FLinearColor(1.2f, 0.05f, 0.05f), A) * Pulse;
			if (Elapsed >= Windup)
			{
				Col = FLinearColor(2.5f, 1.6f, 1.4f); // impact flash
			}
			MID->SetVectorParameterValue(TEXT("Color"), Col);
		}
	}

	// Destroy shortly after the hit lands (a brief flash lingers).
	if (Elapsed >= Windup + 0.12f)
	{
		Destroy();
	}
}
