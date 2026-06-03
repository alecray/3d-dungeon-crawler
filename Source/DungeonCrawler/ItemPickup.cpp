#include "ItemPickup.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/SoftObjectPath.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(90.f);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(Trigger);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.25f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	if (UStaticMesh* Cube = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad()))
	{
		Mesh->SetStaticMesh(Cube);
	}
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnOverlap);
}

void AItemPickup::Configure(FName InItemId, int32 InCount)
{
	ItemId = InItemId;
	Count = FMath::Max(1, InCount);
}

void AItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Gentle spin so dropped items read as pickups.
	Spin += DeltaSeconds * 90.f;
	if (Mesh)
	{
		Mesh->SetRelativeRotation(FRotator(0.f, Spin, 0.f));
	}
}

void AItemPickup::OnOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || ItemId.IsNone())
	{
		return;
	}
	if (UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>())
	{
		const int32 Leftover = Inv->AddItem(ItemId, Count);
		if (Leftover <= 0)
		{
			Destroy();
		}
		else
		{
			Count = Leftover; // inventory full: leave the rest on the ground
		}
	}
}
