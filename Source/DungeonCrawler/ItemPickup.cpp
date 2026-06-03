#include "ItemPickup.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "UObject/SoftObjectPath.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(90.f);
	// Block only the Visibility channel so the player's interact line-trace can target it (and a
	// generous sphere makes it easy to aim at); ignore everything else so it never impedes movement.
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
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

	SkelMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelMesh"));
	SkelMesh->SetupAttachment(Trigger);
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkelMesh->SetVisibility(false);

	Display = Mesh; // graybox cube until Configure() swaps in the item's real mesh
}

void AItemPickup::Configure(FName InItemId, int32 InCount)
{
	ItemId = InItemId;
	Count = FMath::Max(1, InCount);

	if (!ItemDatabase::Contains(ItemId))
	{
		return;
	}
	const FItemDef& Def = ItemDatabase::Get(ItemId);

	// Prefer the item's real icon mesh (so dropped/displayed items look like the item, not a cube).
	UStaticMesh* StaticIcon = !Def.IconStaticMeshPath.IsEmpty()
		? Cast<UStaticMesh>(FSoftObjectPath(Def.IconStaticMeshPath).TryLoad()) : nullptr;
	USkeletalMesh* SkelIcon = (!StaticIcon && !Def.IconSkeletalMeshPath.IsEmpty())
		? Cast<USkeletalMesh>(FSoftObjectPath(Def.IconSkeletalMeshPath).TryLoad()) : nullptr;

	if (!StaticIcon && !SkelIcon)
	{
		return; // no mesh for this item: keep the graybox cube
	}

	FBoxSphereBounds Bounds;
	if (StaticIcon)
	{
		Mesh->SetStaticMesh(StaticIcon);
		Bounds = StaticIcon->GetBounds();
		Display = Mesh;
	}
	else
	{
		Mesh->SetVisibility(false);
		SkelMesh->SetSkeletalMesh(SkelIcon);
		SkelMesh->SetVisibility(true);
		Bounds = SkelIcon->GetBounds();
		Display = SkelMesh;
	}

	// Optional per-item recolor (e.g. health/mana/stamina potions share one mesh+material).
	if (!Def.IconMaterialParam.IsEmpty() && !Def.IconTexturePath.IsEmpty())
	{
		if (UTexture* Tex = Cast<UTexture>(FSoftObjectPath(Def.IconTexturePath).TryLoad()))
		{
			if (UMeshComponent* MC = Cast<UMeshComponent>(Display))
			{
				if (UMaterialInstanceDynamic* MID = MC->CreateDynamicMaterialInstance(0))
				{
					MID->SetTextureParameterValue(FName(Def.IconMaterialParam), Tex);
				}
			}
		}
	}

	// Normalize size so any item reads as a tabletop pickup (~28 cm tall) and rest its base near z=0.
	const float HeightCm = FMath::Max(1.f, Bounds.BoxExtent.Z * 2.f);
	const float Scale = FMath::Clamp(28.f / HeightCm, 0.05f, 4.f);
	Display->SetRelativeScale3D(FVector(Scale));
	const float BottomOffset = (Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale;
	Display->SetRelativeLocation(FVector(0.f, 0.f, -BottomOffset));
}

void AItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Gentle spin so dropped items read as pickups.
	Spin += DeltaSeconds * 90.f;
	if (Display)
	{
		Display->SetRelativeRotation(FRotator(0.f, Spin, 0.f));
	}
}

bool AItemPickup::Collect(APawn* ByPawn)
{
	if (!ByPawn || ItemId.IsNone())
	{
		return false;
	}
	if (UInventoryComponent* Inv = ByPawn->FindComponentByClass<UInventoryComponent>())
	{
		const int32 Leftover = Inv->AddItem(ItemId, Count);
		if (Leftover <= 0)
		{
			Destroy();
			return true;
		}
		Count = Leftover; // inventory full: leave the rest on the ground
	}
	return false;
}
