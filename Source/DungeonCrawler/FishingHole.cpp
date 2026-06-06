#include "FishingHole.h"
#include "FirstPersonCharacter.h"
#include "InventoryComponent.h"
#include "ItemTypes.h"
#include "DungeonGameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

static constexpr float WaterTop = 30.f; // local Z of the water surface

AFishingHole::AFishingHole()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UStaticMesh* Cube = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded()) { CubeMesh = CubeFinder.Object; Cube = CubeFinder.Object; }
	UStaticMesh* Sphere = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded()) { Sphere = SphereFinder.Object; }

	// Real fishing-hole art (the pool/basin). Swaps in over the graybox; null until the model exists.
	UStaticMesh* HoleArt = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HoleFinder(TEXT("/Game/World/SM_Fishing_Hole.SM_Fishing_Hole"));
	if (HoleFinder.Succeeded()) { HoleArt = HoleFinder.Object; }

	HoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoleMesh"));
	HoleMesh->SetupAttachment(Root);
	if (HoleArt) { HoleMesh->SetStaticMesh(HoleArt); }
	HoleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // purely visual; interaction stays on Water
	HoleMesh->SetVisibility(HoleArt != nullptr);

	// Water pool. When the real art is present this becomes an INVISIBLE interact-trace proxy (its known-good
	// box collision keeps the cast/reel line-trace reliable regardless of the art's own collision); when the
	// art is missing it's the blue graybox cube. Blocks Visibility only, so it never walls the player.
	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));
	Water->SetupAttachment(Root);
	if (Cube) { Water->SetStaticMesh(Cube); }
	Water->SetRelativeScale3D(FVector(3.0f, 3.0f, WaterTop / 50.f));
	Water->SetRelativeLocation(FVector(0.f, 0.f, WaterTop * 0.5f));
	Water->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Water->SetCollisionResponseToAllChannels(ECR_Ignore);
	Water->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Water->SetVisibility(HoleArt == nullptr); // hide the graybox cube once the real hole is in

	// Bobber — sits on the water while a line is cast (hidden otherwise).
	Bobber = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bobber"));
	Bobber->SetupAttachment(Root);
	if (Sphere) { Bobber->SetStaticMesh(Sphere); }
	Bobber->SetRelativeScale3D(FVector(0.18f));
	Bobber->SetRelativeLocation(FVector(0.f, 110.f, WaterTop + 8.f));
	Bobber->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bobber->SetVisibility(false);

	// Caught fish — pops up briefly on a catch (graybox).
	CaughtFish = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CaughtFish"));
	CaughtFish->SetupAttachment(Root);
	if (Cube) { CaughtFish->SetStaticMesh(Cube); }
	CaughtFish->SetRelativeScale3D(FVector(0.25f, 0.1f, 0.12f));
	CaughtFish->SetRelativeLocation(FVector(0.f, 60.f, WaterTop + 90.f));
	CaughtFish->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CaughtFish->SetVisibility(false);

	StatusText3D = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText3D"));
	StatusText3D->SetupAttachment(Root);
	StatusText3D->SetRelativeLocation(FVector(0.f, 0.f, WaterTop + 150.f));
	StatusText3D->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	StatusText3D->SetHorizontalAlignment(EHTA_Center);
	StatusText3D->SetWorldSize(22.f);
	StatusText3D->SetTextRenderColor(FColor(160, 220, 255));
	StatusText3D->SetText(FText::FromString(TEXT("Fishing Hole")));

	WaterTopZ = WaterTop;
}

void AFishingHole::BeginPlay()
{
	Super::BeginPlay();
	if (Bobber)
	{
		if (UMaterialInstanceDynamic* MID = Bobber->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.2f, 0.15f)); // red float
		}
	}
	if (Water)
	{
		if (UMaterialInstanceDynamic* MID = Water->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.18f, 0.35f)); // water blue
		}
	}
}

void AFishingHole::SetStatus(const FString& S)
{
	if (StatusText3D) { StatusText3D->SetText(FText::FromString(S)); }
}

FString AFishingHole::GetPrompt() const
{
	switch (State)
	{
	case EState::Waiting: return TEXT("Reel in");
	case EState::Biting:  return TEXT("REEL IN!");
	default:              return TEXT("Cast a line");
	}
}

void AFishingHole::Interact(AFirstPersonCharacter* Player)
{
	Angler = Player;
	switch (State)
	{
	case EState::Idle:
		State = EState::Waiting;
		if (Bobber) { Bobber->SetVisibility(true); }
		if (CaughtFish) { CaughtFish->SetVisibility(false); }
		SetStatus(TEXT("Waiting for a bite..."));
		GetWorldTimerManager().SetTimer(BiteTimer, this, &AFishingHole::StartBite, FMath::FRandRange(MinWait, MaxWait), false);
		break;

	case EState::Waiting:
		GetWorldTimerManager().ClearTimer(BiteTimer);
		State = EState::Idle;
		if (Bobber) { Bobber->SetVisibility(false); }
		SetStatus(TEXT("Reeled in early — nothing."));
		break;

	case EState::Biting:
	{
		const FName Fish = RollFish();
		if (Angler.IsValid())
		{
			if (UInventoryComponent* Inv = Angler->GetInventoryComponent()) { Inv->AddItem(Fish, 1); }
		}
		if (Fish != FName(TEXT("OldBoot"))) // boots don't count as a fish
		{
			if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(UGameplayStatics::GetGameInstance(this)))
			{
				GI->GetStats().FishCaught++;
				GI->SaveProfile();
			}
		}
		State = EState::Idle;
		BiteLeft = 0.f;
		if (Bobber) { Bobber->SetVisibility(false); }
		ShowCatch(Fish);
		break;
	}
	}
}

void AFishingHole::StartBite()
{
	if (State != EState::Waiting) { return; }
	State = EState::Biting;
	BiteLeft = BiteWindow;
	SetStatus(TEXT("It's biting — REEL!"));
}

FName AFishingHole::RollFish() const
{
	// Weighted catch table (out of 100). Skewed to small fry; Golden Carp is the prize.
	struct FEntry { const TCHAR* Id; int32 Weight; };
	static const FEntry Table[] = {
		{ TEXT("OldBoot"), 8 }, { TEXT("Minnow"), 25 }, { TEXT("Sardine"), 23 },
		{ TEXT("Trout"), 18 },  { TEXT("Bass"), 14 },   { TEXT("Pike"), 9 },
		{ TEXT("GoldenCarp"), 3 },
	};
	int32 Roll = FMath::RandRange(0, 99);
	for (const FEntry& E : Table)
	{
		if (Roll < E.Weight) { return FName(E.Id); }
		Roll -= E.Weight;
	}
	return FName(TEXT("Minnow"));
}

void AFishingHole::ShowCatch(FName FishId)
{
	const FString Name = ItemDatabase::Contains(FishId) ? ItemDatabase::Get(FishId).DisplayName : FishId.ToString();
	const bool bJunk = (FishId == FName(TEXT("OldBoot")));
	SetStatus(bJunk ? FString::Printf(TEXT("Hauled up an %s..."), *Name) : FString::Printf(TEXT("Caught a %s!"), *Name));

	if (CaughtFish)
	{
		CaughtFish->SetVisibility(true);
		if (FishMeshOverride) { CaughtFish->SetStaticMesh(FishMeshOverride); }
		if (UMaterialInstanceDynamic* MID = CaughtFish->CreateAndSetMaterialInstanceDynamic(0))
		{
			const FLinearColor Col = ItemDatabase::Contains(FishId) ? RarityColor(ItemDatabase::Get(FishId).Rarity) : FLinearColor::Gray;
			MID->SetVectorParameterValue(TEXT("Color"), Col);
		}
	}
	CatchDisplayLeft = 2.6f;
}

void AFishingHole::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BobPhase += DeltaSeconds;

	if (Bobber && Bobber->IsVisible())
	{
		// Gentle bob while waiting; a fast shake while biting.
		const float Amp = (State == EState::Biting) ? 9.f : 2.5f;
		const float Speed = (State == EState::Biting) ? 22.f : 3.f;
		const float Z = WaterTopZ + 8.f + FMath::Sin(BobPhase * Speed) * Amp;
		Bobber->SetRelativeLocation(FVector(0.f, 110.f, Z));
	}

	if (State == EState::Biting)
	{
		BiteLeft -= DeltaSeconds;
		if (BiteLeft <= 0.f)
		{
			State = EState::Idle;
			if (Bobber) { Bobber->SetVisibility(false); }
			SetStatus(TEXT("It got away..."));
		}
	}

	if (CatchDisplayLeft > 0.f)
	{
		CatchDisplayLeft -= DeltaSeconds;
		if (CaughtFish) { CaughtFish->AddRelativeRotation(FRotator(0.f, 120.f * DeltaSeconds, 0.f)); } // slow spin
		if (CatchDisplayLeft <= 0.f && CaughtFish) { CaughtFish->SetVisibility(false); }
	}
}
