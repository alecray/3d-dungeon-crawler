#include "LootChest.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "UObject/SoftObjectPath.h"

ALootChest::ALootChest()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	// SK_Chest has no physics asset, so the skeletal mesh alone blocks nothing and the interact
	// line-trace passes through it. A box collider (sized + centered to the mesh bounds in
	// ResolveMeshes) provides both player collision and a Visibility target for the [E] Open prompt.
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetupAttachment(SceneRoot);
	Collision->SetCollisionProfileName(TEXT("BlockAll"));
	Collision->SetBoxExtent(FVector(40.f, 30.f, 30.f)); // provisional; refined to mesh bounds at runtime

	// Skeletal chest (mesh/anims assigned at runtime in ResolveMeshes). No collision — the box handles it.
	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh"));
	ChestMesh->SetupAttachment(SceneRoot);
	ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ChestInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("ChestInventory"));
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
	if (!ChestMesh || ChestMesh->GetSkeletalMeshAsset())
	{
		return; // already resolved
	}

	USkeletalMesh* Mesh = Cast<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Furniture/SK_Chest.SK_Chest")).TryLoad());
	if (!Mesh)
	{
		return;
	}
	ChestMesh->SetSkeletalMesh(Mesh);
	ChestMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Wrap the box collider around the mesh so collision + the interact target match the visible chest.
	if (Collision)
	{
		const FBoxSphereBounds B = Mesh->GetBounds();
		Collision->SetBoxExtent(B.BoxExtent);
		Collision->SetRelativeLocation(B.Origin);
	}

	OpenAnim = Cast<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Furniture/A_Chest_Open.A_Chest_Open")).TryLoad());
	CloseAnim = Cast<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Furniture/A_Chest_Close.A_Chest_Close")).TryLoad());

	// Rest in the closed pose until opened.
	if (CloseAnim)
	{
		ChestMesh->PlayAnimation(CloseAnim, /*bLooping*/ false);
	}
}

void ALootChest::Open()
{
	if (!bOpened)
	{
		bOpened = true;
		RollLoot(); // roll once, on first open
	}
	if (ChestMesh && OpenAnim)
	{
		ChestMesh->PlayAnimation(OpenAnim, /*bLooping*/ false); // holds on the final (open) frame
	}
}

void ALootChest::CloseLid()
{
	if (ChestMesh && CloseAnim)
	{
		ChestMesh->PlayAnimation(CloseAnim, /*bLooping*/ false);
	}
}

void ALootChest::RollLoot()
{
	if (!ChestInventory)
	{
		return;
	}

	FRandomStream Rng(FMath::Rand());
	const int32 Drops = Rng.RandRange(FMath::Min(MinDrops, MaxDrops), FMath::Max(MinDrops, MaxDrops));
	for (int32 i = 0; i < Drops; ++i)
	{
		const FName ItemId = ItemDatabase::RollRandomItem(Rng);
		if (ItemId.IsNone())
		{
			continue;
		}
		const int32 Count = (ItemDatabase::Get(ItemId).MaxStack > 1) ? Rng.RandRange(1, 3) : 1;
		ChestInventory->AddItem(ItemId, Count);
	}
}
