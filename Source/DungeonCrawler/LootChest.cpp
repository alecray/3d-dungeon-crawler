#include "LootChest.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "UObject/SoftObjectPath.h"

ALootChest::ALootChest()
{
	PrimaryActorTick.bCanEverTick = false;

	// Skeletal chest (mesh/anims assigned at runtime in ResolveMeshes — loading content in the
	// constructor can return null at CDO time). BlockAll so it stops the player and the interact trace.
	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh"));
	SetRootComponent(ChestMesh);
	ChestMesh->SetCollisionProfileName(TEXT("BlockAll"));

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
	if (bOpened)
	{
		return;
	}
	bOpened = true;

	if (ChestMesh && OpenAnim)
	{
		ChestMesh->PlayAnimation(OpenAnim, /*bLooping*/ false); // holds on the final (open) frame
	}
	RollLoot();
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
