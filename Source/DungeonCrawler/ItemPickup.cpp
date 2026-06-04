#include "ItemPickup.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
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

	// Rarity loot beam (a thin glowing pillar + a colored light); off until Configure() knows the rarity.
	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Trigger);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	if (UStaticMesh* Cyl = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad()))
	{
		Beam->SetStaticMesh(Cyl);
	}
	Beam->SetVisibility(false);

	BeamLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeamLight"));
	BeamLight->SetupAttachment(Trigger);
	BeamLight->SetCastShadows(false);
	BeamLight->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	BeamLight->SetIntensity(0.f);

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

	// Rarity loot beam: color it by rarity; rarer drops get a taller, wider, brighter pillar so they
	// read across the room. (The engine cylinder is 100cm tall / 50cm radius, pivot at its center.)
	{
		const int32 R = FMath::Clamp((int32)Def.Rarity, 0, 4); // Common(0) .. Legendary(4)
		const FLinearColor Col = RarityColor(Def.Rarity);
		const float HeightScale = 2.4f + R * 0.5f;   // ~240cm .. ~440cm tall
		const float WidthScale = 0.10f + R * 0.02f;  // thin pillar, a touch fatter when rarer
		if (Beam)
		{
			Beam->SetVisibility(true);
			Beam->SetRelativeScale3D(FVector(WidthScale, WidthScale, HeightScale));
			Beam->SetRelativeLocation(FVector(0.f, 0.f, HeightScale * 50.f)); // base near the ground
			if (UMaterialInstanceDynamic* MID = Beam->CreateAndSetMaterialInstanceDynamic(0))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Col);
			}
		}
		if (BeamLight)
		{
			BeamLight->SetLightColor(Col);
			BeamLight->SetIntensity(1800.f + R * 1400.f);
			BeamLight->SetAttenuationRadius(190.f + R * 70.f);
		}
	}

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
