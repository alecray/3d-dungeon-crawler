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
#include "Materials/MaterialInterface.h"
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
	// Cylinder gives the water an OVAL footprint (scale X/Y independently for an ellipse) instead of a square.
	UStaticMesh* Cylinder = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylFinder.Succeeded()) { Cylinder = CylFinder.Object; }

	// Real fishing-hole art (the pool/basin). Swaps in over the graybox; null until the model exists.
	UStaticMesh* HoleArt = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HoleFinder(TEXT("/Game/World/SM_Fishing_Hole.SM_Fishing_Hole"));
	if (HoleFinder.Succeeded()) { HoleArt = HoleFinder.Object; }

	HoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoleMesh"));
	HoleMesh->SetupAttachment(Root);
	if (HoleArt) { HoleMesh->SetStaticMesh(HoleArt); }
	// Solid basin: the art blocks the player (so you fish from the edge, not walk through the pond). Uses
	// the mesh's collision — Tools/make_water_material.py sets SM_Fishing_Hole to complex-as-simple so its
	// tris collide even without authored simple primitives. Also serves as the interact line-trace target.
	HoleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HoleMesh->SetCollisionProfileName(TEXT("BlockAll"));
	// ...but stay OUT of the interact line-trace (ECC_Visibility): the cast/reel trace casts the whole actor
	// to AFishingHole, so if the rocks blocked Visibility the entire mesh would be fishable. Ignoring it here
	// lets the trace pass through the art and land only on the Water box below — so only the water is fishable.
	HoleMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	HoleMesh->SetVisibility(HoleArt != nullptr);

	// The model's own water surface (a material slot on SM_Fishing_Hole) gets the M_PondWater shader,
	// applied at construction/runtime in ApplyWaterMaterial() — NOT here, so the CDO never references the
	// material and the build script (Tools/make_water_material.py) can rebuild it freely.

	// Water "pool" box — the VISIBLE water surface (purely visual now). Its oval footprint is sized from
	// WaterExtent (see UpdateWaterBox) and it gets M_PondWater in ApplyWaterMaterial. The fishing interact
	// target is CastVolume below, not this.
	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));
	Water->SetupAttachment(Root);
	if (Cylinder) { Water->SetStaticMesh(Cylinder); } // oval surface
	else if (Cube) { Water->SetStaticMesh(Cube); }    // fallback
	Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// CastVolume — the invisible interact target. A tall box rising CastPromptHeight above the water (same
	// X/Y footprint), so the "Cast a line" prompt shows when looking roughly level at the pond, not only when
	// staring straight down. Blocks ONLY ECC_Visibility (the interact line-trace), never the player.
	CastVolume = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CastVolume"));
	CastVolume->SetupAttachment(Root);
	if (Cube) { CastVolume->SetStaticMesh(Cube); }
	CastVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CastVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	CastVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CastVolume->SetVisibility(false);

	UpdateWaterBox(); // sizes/positions the water + cast volume from WaterExtent

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
	StatusText3D->SetVisibility(false); // fishing status now shows on the HUD, not floating in the world

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
	ApplyWaterMaterial(); // also (re)apply at runtime in case the asset wasn't present at construction
}

void AFishingHole::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateWaterBox();     // re-size the surface live as WaterExtent is tuned in the editor
	ApplyWaterMaterial(); // so the water shows in-editor too
}

void AFishingHole::UpdateWaterBox()
{
	// X/Y from the tunable footprint (cm -> unit-mesh scale; the BasicShapes Cylinder/Cube are 100cm across,
	// so scale = extent/100 gives that diameter).
	const float SX = FMath::Max(WaterExtent.X, 1.f) / 100.f;
	const float SY = FMath::Max(WaterExtent.Y, 1.f) / 100.f;
	if (Water)
	{
		// A thin disc sits with its TOP at WaterHeight, so raising WaterHeight lifts the surface.
		const float Thickness = 12.f; // cm
		Water->SetRelativeScale3D(FVector(SX, SY, Thickness / 100.f));
		Water->SetRelativeLocation(FVector(0.f, 0.f, WaterHeight - Thickness * 0.5f));
		Water->SetVisibility(true);
	}
	if (CastVolume)
	{
		// Tall invisible interact box rising from the water surface upward by CastPromptHeight.
		const float H = FMath::Max(CastPromptHeight, 20.f);
		CastVolume->SetRelativeScale3D(FVector(SX, SY, H / 100.f));
		CastVolume->SetRelativeLocation(FVector(0.f, 0.f, WaterHeight + H * 0.5f));
	}
	WaterTopZ = WaterHeight; // bobber / cast logic ride the current surface height
}

void AFishingHole::ApplyWaterMaterial()
{
	// Runtime load (not a CDO FObjectFinder) so M_PondWater is never rooted by this class — that keeps it
	// editable by Tools/make_water_material.py. Null until that script has run; then it just appears.
	UMaterialInterface* WaterMat = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/World/M_PondWater.M_PondWater")).TryLoad());
	if (!WaterMat)
	{
		return;
	}
	// Wrap M_PondWater in a dynamic instance so WaterColor can tint it: ShallowColor = WaterColor, DeepColor a
	// darker shade for depth. Fall back to the raw material if the MID can't be made.
	UMaterialInterface* Applied = WaterMat;
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(WaterMat, this))
	{
		MID->SetVectorParameterValue(TEXT("ShallowColor"), WaterColor);
		FLinearColor Deep = WaterColor * 0.35f;
		Deep.A = 1.f;
		MID->SetVectorParameterValue(TEXT("DeepColor"), Deep);
		Applied = MID;
	}
	// The visible water is the Water box — shade it (independent of the art being present).
	if (Water)
	{
		Water->SetMaterial(0, Applied);
	}
	// Also shade the model's own water face (under the box) so nothing peeks through as a placeholder. Clear
	// prior overrides first so flipping WaterMaterialSlot doesn't strand the shader on the old slot.
	if (HoleMesh && HoleMesh->GetStaticMesh())
	{
		const int32 NumSlots = HoleMesh->GetNumMaterials();
		for (int32 i = 0; i < NumSlots; ++i)
		{
			HoleMesh->SetMaterial(i, nullptr);
		}
		if (WaterMaterialSlot >= 0 && WaterMaterialSlot < NumSlots)
		{
			HoleMesh->SetMaterial(WaterMaterialSlot, Applied);
		}
	}
}

void AFishingHole::SetStatus(const FString& S)
{
	if (Angler.IsValid()) { Angler->SetFishingStatus(S); }       // on-screen HUD line
	if (StatusText3D) { StatusText3D->SetText(FText::FromString(S)); } // kept in sync (hidden in-world by default)
}

FString AFishingHole::GetPrompt() const
{
	switch (State)
	{
	case EState::Waiting: return TEXT("Reel in");
	case EState::Biting:  return TEXT("REEL IN!");
	case EState::Tension: return FString();       // auto-resolving — no interact prompt
	default:              return TEXT("Cast a line");
	}
}

void AFishingHole::Interact(AFirstPersonCharacter* Player)
{
	Angler = Player;
	switch (State)
	{
	case EState::Idle:
	{
		// A rod is required to cast.
		if (Player && !Player->HasFishingRod())
		{
			SetStatus(TEXT("You need a Fishing Rod to cast a line."));
			break;
		}
		State = EState::Waiting;
		if (Bobber) { Bobber->SetVisibility(true); }
		if (CaughtFish) { CaughtFish->SetVisibility(false); }
		if (Player)
		{
			Player->BeginFishing();
			Player->PlayFishingPose(EFishingPose::Cast);
			// settle from the cast animation into the waiting/idle loop after a beat
			FTimerDelegate ToIdle = FTimerDelegate::CreateUObject(Player, &AFirstPersonCharacter::PlayFishingPose, EFishingPose::Idle);
			GetWorldTimerManager().SetTimer(CastIdleTimer, ToIdle, FMath::Max(CastAnimTime, 0.05f), false);
		}
		SetStatus(TEXT("You cast your line..."));
		GetWorldTimerManager().SetTimer(BiteTimer, this, &AFishingHole::StartBite, FMath::FRandRange(MinWait, MaxWait), false);
		break;
	}

	case EState::Waiting:
		// Reel in early — nothing on the line.
		GetWorldTimerManager().ClearTimer(BiteTimer);
		GetWorldTimerManager().ClearTimer(CastIdleTimer);
		State = EState::Idle;
		if (Bobber) { Bobber->SetVisibility(false); }
		FinishFishing(TEXT("Reeled in early — nothing."));
		break;

	case EState::Biting:
		// Hooked it! The tension struggle now decides catch vs fail over a random delay.
		GetWorldTimerManager().ClearTimer(CastIdleTimer);
		State = EState::Tension;
		BiteLeft = 0.f;
		if (Angler.IsValid()) { Angler->PlayFishingPose(EFishingPose::Tension); }
		SetStatus(TEXT("Something's on the line — hold on!"));
		GetWorldTimerManager().SetTimer(TensionTimer, this, &AFishingHole::ResolveCatch, FMath::FRandRange(TensionMin, TensionMax), false);
		break;

	case EState::Tension:
		break; // auto-resolving — ignore further presses
	}
}

void AFishingHole::StartBite()
{
	if (State != EState::Waiting) { return; }
	State = EState::Biting;
	BiteLeft = BiteWindow;
	SetStatus(TEXT("It's biting — REEL!"));
}

void AFishingHole::ResolveCatch()
{
	if (State != EState::Tension) { return; }
	State = EState::Idle;
	if (Bobber) { Bobber->SetVisibility(false); }

	if (FMath::FRand() <= CatchChance)
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
		ShowCatch(Fish);          // sets the "Caught a X!" status + shows the fish
		FinishFishing(FString()); // keep ShowCatch's status; just reel-in + stow the rod
	}
	else
	{
		FinishFishing(TEXT("It got away..."));
	}
}

void AFishingHole::FinishFishing(const FString& EndStatus)
{
	if (!EndStatus.IsEmpty()) { SetStatus(EndStatus); }
	if (Angler.IsValid())
	{
		Angler->PlayFishingPose(EFishingPose::Reel);
		// Reel-in plays, then stow the rod (restore the equipped weapon) after a beat.
		TWeakObjectPtr<AFirstPersonCharacter> A = Angler;
		FTimerDelegate Stow = FTimerDelegate::CreateLambda([A]() { if (A.IsValid()) { A->EndFishing(); } });
		GetWorldTimerManager().SetTimer(EndTimer, Stow, FMath::Max(ReelAnimTime, 0.1f), false);
	}
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
		// Gentle bob while waiting; a fast shake while a fish is biting or fighting on the line.
		const bool bAgitated = (State == EState::Biting || State == EState::Tension);
		const float Amp = bAgitated ? 9.f : 2.5f;
		const float Speed = bAgitated ? 22.f : 3.f;
		const float Z = WaterTopZ + 8.f + FMath::Sin(BobPhase * Speed) * Amp;
		Bobber->SetRelativeLocation(FVector(0.f, 110.f, Z));
	}

	if (State == EState::Biting)
	{
		BiteLeft -= DeltaSeconds;
		if (BiteLeft <= 0.f)
		{
			// Missed the reel window — the fish slips off.
			GetWorldTimerManager().ClearTimer(CastIdleTimer);
			State = EState::Idle;
			if (Bobber) { Bobber->SetVisibility(false); }
			FinishFishing(TEXT("It got away..."));
		}
	}

	if (CatchDisplayLeft > 0.f)
	{
		CatchDisplayLeft -= DeltaSeconds;
		if (CaughtFish) { CaughtFish->AddRelativeRotation(FRotator(0.f, 120.f * DeltaSeconds, 0.f)); } // slow spin
		if (CatchDisplayLeft <= 0.f && CaughtFish) { CaughtFish->SetVisibility(false); }
	}
}
