#include "ShopNPC.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"

AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
	Body->InitCapsuleSize(45.f, 90.f);
	// Query-only, block only Visibility so the player's interact line-trace can hit it.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(Body);

	// Graybox stand-in body (swap for a real NPC mesh later).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Body);
	Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* Cyl = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad()))
	{
		Mesh->SetStaticMesh(Cyl);
	}
}

const TArray<FName>& AShopNPC::GetWares() const
{
	if (Wares.Num() > 0)
	{
		return Wares;
	}
	if (DefaultStock.Num() == 0)
	{
		DefaultStock = {
			FName(TEXT("HealthPotion")), FName(TEXT("ManaPotion")), FName(TEXT("StaminaPotion")),
			FName(TEXT("Sword")), FName(TEXT("Crossbow")), FName(TEXT("Wand")), FName(TEXT("IronSword")),
			FName(TEXT("LeatherArmor")), FName(TEXT("FishingRod")),
			FName(TEXT("DeathScroll"))
		};
	}
	return DefaultStock;
}
