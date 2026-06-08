#include "BlackjackTable.h"
#include "BlackjackWidget.h"
#include "FirstPersonCharacter.h"
#include "DungeonGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

// Local-space layout (cm). TableTopZ is only the graybox fallback height; with real art the felt
// surface is read from the mesh bounds (see TableSurfaceZ). Cards lie flat just above the surface.
static constexpr float TableTopZ = 70.f;
static constexpr float CardSpacingX = 24.f; // > card width (20cm) so cards in a row don't overlap
static constexpr float RowInset = 36.f; // player row sits this far in from the felt's near edge (pushed onto the felt)
static constexpr float RowGap = 40.f;   // dealer row sits this far back from the player row (kept close)
static constexpr float ValueLabelSideX = 48.f; // how far left of the cards the "Dealer N"/"You N" labels sit
static constexpr float SeatStandoff = 30.f;   // how far in front of the felt's near edge the player is seated
static constexpr float SeatPitch = -20.f;     // fixed downward look angle onto the felt (tilted up 10° from -30)
static constexpr float SeatRightOffset = 0.f; // lateral shift of the seat (+X local = player's right); 0 = centered

ABlackjackTable::ABlackjackTable()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded()) { CubeMesh = CubeFinder.Object; }

	// Cards are textured planes: a flat 100x100 quad (normal +Z) showing the card face.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded()) { PlaneMesh = PlaneFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardMatFinder(TEXT("/Game/Cards/M_Card.M_Card"));
	if (CardMatFinder.Succeeded()) { CardMaterial = CardMatFinder.Object; }

	// Finished table art. The mesh is a D-shape: flat dealer edge at local -X, curved player edge at +X.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TableArt(TEXT("/Game/Furniture/SM_Blackjack_Table.SM_Blackjack_Table"));
	if (TableArt.Succeeded()) { TableMeshOverride = TableArt.Object; }

	// Felt slab — blocks the player + is the interact target.
	Table = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Table"));
	Table->SetupAttachment(Root);
	if (TableMeshOverride)
	{
		// Yaw +90 turns the curved edge to face +Y (the player/approach side), flat edge away. Offset the
		// mesh down by its local base so the bottom sits at the actor origin — keeps it on the floor even
		// though the art's pivot was moved off the base.
		Table->SetStaticMesh(TableMeshOverride);
		Table->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		Table->SetRelativeScale3D(FVector(1.f));
		const FBox MeshBox = TableMeshOverride->GetBoundingBox();
		Table->SetRelativeLocation(FVector(0.f, 0.f, -MeshBox.Min.Z));
	}
	else if (CubeMesh)
	{
		// Graybox fallback.
		Table->SetStaticMesh(CubeMesh);
		Table->SetRelativeScale3D(FVector(1.6f, 1.4f, TableTopZ / 50.f)); // ~160 x 140, top at TableTopZ
		Table->SetRelativeLocation(FVector(0.f, 0.f, TableTopZ * 0.5f));
	}
	Table->SetCollisionEnabled(ECollisionEnabled::NoCollision); // visual only — the box below does the blocking

	// Blocking + interact-trace volume. Independent of the mesh's authored collision/pivot; sized to the real
	// table bounds in BeginPlay so it lines up even after the model's origin was moved.
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetupAttachment(Root);
	Collision->SetCollisionProfileName(TEXT("BlockAll"));
	Collision->SetBoxExtent(FVector(80.f, 70.f, 35.f)); // placeholder; resized to real bounds in BeginPlay

	// Hand-value labels sitting beside each card row (positioned to the real felt in BeginPlay).
	DealerValueText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DealerValueText"));
	DealerValueText->SetupAttachment(Root);
	DealerValueText->SetHorizontalAlignment(EHTA_Center);
	DealerValueText->SetWorldSize(16.f);
	DealerValueText->SetTextRenderColor(FColor(255, 250, 220));
	DealerValueText->SetText(FText::GetEmpty());

	PlayerValueText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PlayerValueText"));
	PlayerValueText->SetupAttachment(Root);
	PlayerValueText->SetHorizontalAlignment(EHTA_Center);
	PlayerValueText->SetWorldSize(16.f);
	PlayerValueText->SetTextRenderColor(FColor(255, 250, 220));
	PlayerValueText->SetText(FText::GetEmpty());
}

void ABlackjackTable::BeginPlay()
{
	Super::BeginPlay();

	// Fit the blocking/interact box to the real table bounds (independent of the mesh's moved origin).
	if (Collision)
	{
		const FBox Felt = TableBoundsActor();
		Collision->SetBoxExtent(Felt.GetExtent().ComponentMax(FVector(10.f)), /*bUpdateOverlaps*/ false);
		Collision->SetRelativeLocation(Felt.GetCenter());
	}

	const FBox Felt = TableBoundsActor();

	// Hand-value labels just left of each card row, standing up facing the player (+Y).
	{
		const float PlayerY = Felt.Max.Y - RowInset;
		const float DealerY = FMath::Max(PlayerY - RowGap, Felt.Min.Y + RowInset);
		const float SideX = Felt.GetCenter().X - ValueLabelSideX;
		const float LabelZ = Felt.Max.Z + 4.f;
		if (DealerValueText)
		{
			DealerValueText->SetRelativeLocation(FVector(SideX, DealerY, LabelZ));
			DealerValueText->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		}
		if (PlayerValueText)
		{
			PlayerValueText->SetRelativeLocation(FVector(SideX, PlayerY, LabelZ));
			PlayerValueText->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		}
	}
}

void ABlackjackTable::Play(AFirstPersonCharacter* InPlayer)
{
	if (!InPlayer) { return; }
	APlayerController* PC = Cast<APlayerController>(InPlayer->GetController());
	if (!PC) { return; }

	Player = InPlayer;
	InPlayer->SetBlackjackActive(true); // hide the "Play Blackjack" prompt while seated
	Phase = EPhase::Betting;
	PlayerHand.Reset();
	DealerHand.Reset();
	ClearCardVisuals();
	Status = TEXT("Place your bet and Deal.");

	// Pull the player to a fixed seat in front of the table so the camera framing is consistent every time.
	{
		const FBox Felt = TableBoundsActor();
		const FTransform TableXf = GetActorTransform();
		const float SeatX = Felt.GetCenter().X + SeatRightOffset; // shifted right; look target shifts with it (pure pan)

		FVector WorldSeat = TableXf.TransformPosition(FVector(SeatX, Felt.Max.Y + SeatStandoff, 0.f));
		WorldSeat.Z = InPlayer->GetActorLocation().Z; // keep them standing on the floor they're already on
		InPlayer->SetActorLocation(WorldSeat, false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Move = InPlayer->GetCharacterMovement()) { Move->StopMovementImmediately(); }

		// Look at the felt (same right offset so it's a clean lateral move), locked to a consistent angle.
		const FVector FeltCenterWorld = TableXf.TransformPosition(FVector(SeatX, Felt.GetCenter().Y, Felt.Max.Z));
		FRotator Look = (FeltCenterWorld - WorldSeat).Rotation();
		Look.Pitch = SeatPitch;
		Look.Roll = 0.f;
		PC->SetControlRotation(Look);
	}

	const int32 Gold = GetGold();
	Bet = FMath::Clamp(Bet, 5, FMath::Max(5, Gold));

	if (!Widget)
	{
		Widget = CreateWidget<UBlackjackWidget>(PC, UBlackjackWidget::StaticClass());
	}
	if (Widget)
	{
		Widget->OpenFor(this, PC);
	}
	RefreshWidget();
}

int32 ABlackjackTable::GetGold() const
{
	return Player.IsValid() ? Player->GetGold() : 0;
}

FBox ABlackjackTable::TableBoundsActor() const
{
	if (Table && Table->GetStaticMesh())
	{
		// Transform the mesh's local box by the component's transform so we get the felt's real extents
		// in the actor's frame (the art mesh is rotated/offset, so its felt isn't centered on the origin).
		return Table->GetStaticMesh()->GetBoundingBox().TransformBy(Table->GetRelativeTransform());
	}
	return FBox(FVector(-80.f, -70.f, 0.f), FVector(80.f, 70.f, TableTopZ)); // graybox-ish fallback
}

FBJCard ABlackjackTable::DrawCard() const
{
	FBJCard Card;
	Card.Rank = FMath::RandRange(1, 13); // infinite shoe (fine for a prototype)
	Card.Suit = FMath::RandRange(0, 3);
	return Card;
}

int32 ABlackjackTable::HandValue(const TArray<FBJCard>& Hand)
{
	int32 Total = 0, Aces = 0;
	for (const FBJCard& Card : Hand)
	{
		if (Card.Rank == 1) { Aces++; Total += 11; }
		else { Total += FMath::Min(Card.Rank, 10); }
	}
	while (Total > 21 && Aces > 0) { Total -= 10; Aces--; }
	return Total;
}

bool ABlackjackTable::IsBlackjack(const TArray<FBJCard>& Hand)
{
	return Hand.Num() == 2 && HandValue(Hand) == 21;
}

FString ABlackjackTable::CardName(int32 Rank)
{
	switch (Rank)
	{
	case 1:  return TEXT("A");
	case 11: return TEXT("J");
	case 12: return TEXT("Q");
	case 13: return TEXT("K");
	default: return FString::FromInt(Rank);
	}
}

FString ABlackjackTable::CardTextureName(const FBJCard& Card)
{
	static const TCHAR* Suits[] = { TEXT("clubs"), TEXT("diamonds"), TEXT("hearts"), TEXT("spades") };
	const int32 S = FMath::Clamp(Card.Suit, 0, 3);
	return FString::Printf(TEXT("%s_%s"), Suits[S], *CardName(Card.Rank)); // e.g. "hearts_A"
}

UTexture2D* ABlackjackTable::GetCardTexture(const FString& Stem)
{
	if (const TObjectPtr<UTexture2D>* Found = TextureCache.Find(Stem))
	{
		return *Found;
	}

	// PNGs live next to the other source art, loaded straight off disk (no import step needed).
	const FString Path = FPaths::ProjectContentDir() / TEXT("Images/Cards") / (Stem + TEXT(".png"));
	UTexture2D* Tex = nullptr;

	TArray<uint8> Raw;
	if (FFileHelper::LoadFileToArray(Raw, *Path))
	{
		IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
		TArray<uint8> Bgra;
		if (Wrapper.IsValid() && Wrapper->SetCompressed(Raw.GetData(), Raw.Num())
			&& Wrapper->GetRaw(ERGBFormat::BGRA, 8, Bgra))
		{
			const int32 W = Wrapper->GetWidth();
			const int32 H = Wrapper->GetHeight();
			Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
			if (Tex)
			{
				Tex->SRGB = true;
				void* Dest = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(Dest, Bgra.GetData(), Bgra.Num());
				Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
				Tex->UpdateResource();
			}
		}
	}

	if (!Tex)
	{
		UE_LOG(LogTemp, Warning, TEXT("Blackjack: could not load card texture '%s'"), *Path);
	}
	TextureCache.Add(Stem, Tex); // cache nulls too, so we don't re-hit a missing file every deal
	return Tex;
}

void ABlackjackTable::BetDown()
{
	if (!CanBet()) { return; }
	Bet = FMath::Max(5, Bet - 5);
	RefreshWidget();
}

void ABlackjackTable::BetUp()
{
	if (!CanBet()) { return; }
	Bet = FMath::Clamp(Bet + 5, 5, FMath::Max(5, GetGold()));
	RefreshWidget();
}

void ABlackjackTable::Deal()
{
	if (Phase == EPhase::PlayerTurn || !Player.IsValid()) { return; }

	if (!Player->TrySpendGold(Bet))
	{
		Status = TEXT("Not enough gold for that bet.");
		RefreshWidget();
		return;
	}

	PlayerHand.Reset();
	DealerHand.Reset();
	PlayerHand.Add(DrawCard());
	DealerHand.Add(DrawCard());
	PlayerHand.Add(DrawCard());
	DealerHand.Add(DrawCard());

	Phase = EPhase::PlayerTurn;
	Status = TEXT("Hit or Stand.");
	RebuildCardVisuals();

	if (IsBlackjack(PlayerHand)) { DealerPlayAndResolve(); return; }

	RefreshWidget();
}

void ABlackjackTable::Hit()
{
	if (!CanHitStand()) { return; }
	PlayerHand.Add(DrawCard());
	RebuildCardVisuals();
	if (HandValue(PlayerHand) > 21)
	{
		Phase = EPhase::Resolved;
		Status = FString::Printf(TEXT("Bust! You lose %d."), Bet);
		RecordHandStat(/*Payout*/ 0); // bust = loss
	}
	RefreshWidget();
}

void ABlackjackTable::Stand()
{
	if (!CanHitStand()) { return; }
	DealerPlayAndResolve();
}

void ABlackjackTable::DealerPlayAndResolve()
{
	while (HandValue(DealerHand) < 17) { DealerHand.Add(DrawCard()); }

	const int32 P = HandValue(PlayerHand);
	const int32 D = HandValue(DealerHand);
	const bool bPlayerBJ = IsBlackjack(PlayerHand);
	const bool bDealerBJ = IsBlackjack(DealerHand);

	int32 Payout = 0; // total returned (bet was deducted at deal)
	if (P > 21)                       { Status = FString::Printf(TEXT("Bust! You lose %d."), Bet); }
	else if (bPlayerBJ && !bDealerBJ) { Payout = Bet + (Bet * 3) / 2; Status = FString::Printf(TEXT("Blackjack! +%d"), Payout - Bet); }
	else if (bDealerBJ && !bPlayerBJ) { Status = FString::Printf(TEXT("Dealer blackjack. -%d"), Bet); }
	else if (D > 21 || P > D)         { Payout = Bet * 2; Status = FString::Printf(TEXT("You win! +%d"), Bet); }
	else if (P == D)                  { Payout = Bet; Status = TEXT("Push — bet returned."); }
	else                              { Status = FString::Printf(TEXT("Dealer wins. -%d"), Bet); }

	if (Payout > 0 && Player.IsValid()) { Player->AddGold(Payout); }
	RecordHandStat(Payout);

	Phase = EPhase::Resolved;
	RebuildCardVisuals(); // reveal the dealer hole card
	RefreshWidget();
}

void ABlackjackTable::RecordHandStat(int32 Payout)
{
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		FPlayerStats& S = GI->GetStats();
		S.BlackjackHandsPlayed++;
		if (Payout == 0)        { S.BlackjackHandsLost++; } // loss
		else if (Payout > Bet)  { S.BlackjackHandsWon++; }  // win (payout exceeds the returned bet)
		// Payout == Bet -> push (counts as played, neither win nor loss)
		GI->SaveProfile();
	}
}

void ABlackjackTable::ClearCardVisuals()
{
	for (UStaticMeshComponent* M : CardMeshes) { if (M) { M->DestroyComponent(); } }
	CardMeshes.Reset();
}

void ABlackjackTable::SpawnCard(const FBJCard& Card, int32 IndexInRow, int32 RowCount, bool bDealerRow, bool bFaceDown)
{
	// Cards: a 242x340 image (h/w ~= 1.405). Plane is 100x100, so scale to ~20 x 28.1 cm.
	constexpr float CardW = 0.20f;
	constexpr float CardH = CardW * 340.f / 242.f;

	// Place rows from the real felt extents: player row nearest the player (+Y/camera edge), dealer row a
	// fixed gap further back (kept close to the player's cards), clamped to stay on the felt. Centered in X.
	const FBox Felt = TableBoundsActor();
	const float PlayerY = Felt.Max.Y - RowInset;
	const float DealerY = FMath::Max(PlayerY - RowGap, Felt.Min.Y + RowInset);
	const float Y = bDealerRow ? DealerY : PlayerY;
	// Newest card (highest index) sits at the left slot; older cards shift right. So a freshly dealt card
	// always appears on the left and pushes the rest over.
	const float X = Felt.GetCenter().X + (RowCount - 1 - IndexInRow) * CardSpacingX;
	const FVector Loc(X, Y, Felt.Max.Z + 2.f); // laid flat just above the felt

	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetupAttachment(Root);
	M->RegisterComponent();
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCastShadow(false);
	if (PlaneMesh) { M->SetStaticMesh(PlaneMesh); }
	M->SetRelativeLocation(Loc);
	// Plane lies flat (normal +Z) face-up. Yaw turns the card so its top points away from the player
	// (+Y), i.e. readable from the approach side. Flip this 180 if cards read upside-down.
	M->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	M->SetRelativeScale3D(FVector(CardW, CardH, 1.f));

	UTexture2D* Tex = GetCardTexture(bFaceDown ? TEXT("back") : CardTextureName(Card));
	if (CardMaterial)
	{
		if (UMaterialInstanceDynamic* MID = M->CreateDynamicMaterialInstance(0, CardMaterial))
		{
			if (Tex) { MID->SetTextureParameterValue(TEXT("CardTex"), Tex); }
		}
	}
	CardMeshes.Add(M);
}

void ABlackjackTable::RebuildCardVisuals()
{
	ClearCardVisuals();

	// Dealer hole card (2nd) is hidden during the player's turn.
	const bool bHideHole = (Phase == EPhase::PlayerTurn);
	for (int32 i = 0; i < DealerHand.Num(); ++i)
	{
		SpawnCard(DealerHand[i], i, DealerHand.Num(), /*bDealerRow*/ true, /*bFaceDown*/ bHideHole && i == 1);
	}
	for (int32 i = 0; i < PlayerHand.Num(); ++i)
	{
		SpawnCard(PlayerHand[i], i, PlayerHand.Num(), /*bDealerRow*/ false, /*bFaceDown*/ false);
	}
}

void ABlackjackTable::RefreshValueLabels()
{
	const bool bHasHand = PlayerHand.Num() > 0;

	if (PlayerValueText)
	{
		PlayerValueText->SetVisibility(bHasHand);
		if (bHasHand)
		{
			PlayerValueText->SetText(FText::FromString(FString::Printf(TEXT("You %d"), HandValue(PlayerHand))));
		}
	}

	if (DealerValueText)
	{
		DealerValueText->SetVisibility(bHasHand);
		if (bHasHand)
		{
			// During the player's turn the dealer's 2nd card is face-down, so show only the up-card value.
			const bool bReveal = (Phase != EPhase::PlayerTurn);
			const int32 DVal = (!bReveal && DealerHand.Num() > 0)
				? HandValue(TArray<FBJCard>{ DealerHand[0] })
				: HandValue(DealerHand);
			DealerValueText->SetText(FText::FromString(FString::Printf(TEXT("Dealer %d"), DVal)));
		}
	}
}

void ABlackjackTable::RefreshWidget()
{
	if (Widget) { Widget->RefreshFromTable(); }
	RefreshValueLabels();
}

void ABlackjackTable::Leave()
{
	if (Widget) { Widget->CloseBar(); }
	if (Player.IsValid()) { Player->SetBlackjackActive(false); } // restore the interact prompt
	ClearCardVisuals();
	Phase = EPhase::Betting;
	PlayerHand.Reset();
	DealerHand.Reset();
	RefreshValueLabels(); // hide the value labels
}
