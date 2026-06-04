#include "LootChest.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"
#include "ImpactBurst.h"

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
	const bool bFirstOpen = !bOpened;
	if (bFirstOpen)
	{
		bOpened = true;
		RollLoot(); // roll once, on first open
	}
	if (ChestMesh && OpenAnim)
	{
		ChestMesh->PlayAnimation(OpenAnim, /*bLooping*/ false); // holds on the final (open) frame
	}
	if (bFirstOpen)
	{
		SpawnLootSparkle(); // rarity-colored pop on the reveal
	}
}

void ALootChest::SpawnLootSparkle()
{
	UWorld* World = GetWorld();
	if (!World || !bHasLoot)
	{
		return;
	}
	const FLinearColor Col = RarityColor(BestRolledRarity);
	const FVector Loc = GetActorLocation() + FVector(0.f, 0.f, 45.f);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// One burst, plus an extra for the rarer tiers so good loot really pops.
	const int32 Bursts = ((int32)BestRolledRarity >= (int32)EItemRarity::Rare) ? 2 : 1;
	for (int32 i = 0; i < Bursts; ++i)
	{
		if (AImpactBurst* Sparkle = World->SpawnActor<AImpactBurst>(AImpactBurst::StaticClass(), FTransform(Loc), P))
		{
			Sparkle->SetColor(Col);
		}
	}
}

bool ALootChest::IsViewerInFront(const FVector& ViewerLocation) const
{
	// SK_Chest's opening faces its +Y (the actor's right), so that's the "front" to open from. Allow
	// only within a ~60-degree arc of it.
	FVector ToViewer = ViewerLocation - GetActorLocation();
	ToViewer.Z = 0.f;
	if (!ToViewer.Normalize())
	{
		return true; // viewer right on top of it: don't block
	}
	return FVector::DotProduct(GetActorRightVector(), ToViewer) >= 0.5f;
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
		const FItemDef& Def = ItemDatabase::Get(ItemId);
		const int32 Count = (Def.MaxStack > 1) ? Rng.RandRange(1, 3) : 1;
		ChestInventory->AddItem(ItemId, Count);

		// Track the best rarity rolled, for the open-reveal sparkle color.
		bHasLoot = true;
		if ((int32)Def.Rarity > (int32)BestRolledRarity) { BestRolledRarity = Def.Rarity; }
	}
}
