#include "Portal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Graybox arch: a tall, thin slab the player aims at. Blocks only Visibility so the interact
	// trace hits it without impeding movement.
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(Root);
	Frame->SetRelativeScale3D(FVector(0.4f, 2.4f, 3.0f));
	Frame->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	Frame->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Frame->SetCollisionResponseToAllChannels(ECR_Ignore);
	Frame->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	if (UStaticMesh* Cube = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad()))
	{
		Frame->SetStaticMesh(Cube);
	}

	// Cyan glow so portals read as active travel points.
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	Glow->SetLightColor(FLinearColor(0.3f, 0.8f, 1.0f));
	Glow->SetIntensity(4000.f);
	Glow->SetAttenuationRadius(600.f);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();
	SetActive(bActive);
}

void APortal::SetActive(bool bInActive)
{
	bActive = bInActive;
	// Hidden + non-collidable when dormant, so it can't be seen or used until switched on.
	SetActorHiddenInGame(!bActive);
	if (Frame)
	{
		Frame->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (Glow)
	{
		Glow->SetVisibility(bActive);
	}
}
