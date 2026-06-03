#include "LootChest.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/SoftObjectPath.h"

ALootChest::ALootChest()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Base box body (meshes are assigned at runtime in ResolveMeshes — loading content in the
	// constructor can return null at CDO time, which is why the chest looked invisible).
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));
	BaseMesh->SetRelativeLocation(FVector(0.f, 0.f, 25.f));

	// Lid pivots at the back-top edge so it swings open.
	LidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LidPivot"));
	LidPivot->SetupAttachment(Root);
	LidPivot->SetRelativeLocation(FVector(-35.f, 0.f, 50.f));

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(LidPivot);
	LidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LidMesh->SetRelativeLocation(FVector(35.f, 0.f, 6.f));
}

void ALootChest::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ResolveMeshes();
}

void ALootChest::BeginPlay()
{
	Super::BeginPlay();
	ResolveMeshes();
}

void ALootChest::ResolveMeshes()
{
	if (BaseMesh && BaseMesh->GetStaticMesh())
	{
		return; // already resolved
	}

	// Prefer the crate mesh; fall back to the engine cube.
	UStaticMesh* Mesh = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Furniture/SM_Crate.SM_Crate")).TryLoad());
	const bool bIsCrate = (Mesh != nullptr);
	if (!Mesh)
	{
		Mesh = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());
	}
	if (!Mesh)
	{
		return;
	}

	if (BaseMesh)
	{
		BaseMesh->SetStaticMesh(Mesh);
		// Size the engine cube into a chest-ish box; the crate keeps its own size.
		BaseMesh->SetRelativeScale3D(bIsCrate ? FVector::OneVector : FVector(0.7f, 0.5f, 0.5f));
	}
	if (LidMesh)
	{
		LidMesh->SetStaticMesh(Mesh);
		LidMesh->SetRelativeScale3D(bIsCrate ? FVector(1.0f, 1.0f, 0.25f) : FVector(0.7f, 0.52f, 0.12f));
	}
}

void ALootChest::Interact(AActor* Interactor)
{
	if (bOpened || !Interactor)
	{
		return;
	}
	UInventoryComponent* Inventory = Interactor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return;
	}
	bOpened = true;

	// Flip the lid open.
	if (LidPivot)
	{
		LidPivot->SetRelativeRotation(FRotator(-105.f, 0.f, 0.f));
	}

	// Roll loot into the inventory.
	FRandomStream Rng(FMath::Rand());
	const int32 Drops = Rng.RandRange(FMath::Min(MinDrops, MaxDrops), FMath::Max(MinDrops, MaxDrops));
	FString Summary;
	for (int32 i = 0; i < Drops; ++i)
	{
		const FName ItemId = ItemDatabase::RollRandomItem(Rng);
		if (ItemId.IsNone())
		{
			continue;
		}
		const int32 Count = (ItemDatabase::Get(ItemId).MaxStack > 1) ? Rng.RandRange(1, 3) : 1;
		Inventory->AddItem(ItemId, Count);
		Summary += FString::Printf(TEXT("%s x%d  "), *ItemDatabase::Get(ItemId).DisplayName, Count);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Looted: %s"), *Summary));
	}
}
