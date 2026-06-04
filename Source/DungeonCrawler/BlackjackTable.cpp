#include "BlackjackTable.h"
#include "BlackjackWidget.h"
#include "FirstPersonCharacter.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

// Local-space layout (cm). The felt top sits at Z = TableTopZ; cards lie flat just above it.
static constexpr float TableTopZ = 70.f;
static constexpr float CardZ = TableTopZ + 2.f;
static constexpr float RowY = 55.f;     // dealer row at +Y, player row at -Y
static constexpr float CardSpacingX = 17.f;

ABlackjackTable::ABlackjackTable()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded()) { CubeMesh = CubeFinder.Object; }

	// Felt slab — blocks the player + is the interact target.
	Table = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Table"));
	Table->SetupAttachment(Root);
	if (CubeMesh) { Table->SetStaticMesh(CubeMesh); }
	Table->SetRelativeScale3D(FVector(1.6f, 1.4f, TableTopZ / 50.f)); // ~160 x 140, top at TableTopZ
	Table->SetRelativeLocation(FVector(0.f, 0.f, TableTopZ * 0.5f));
	Table->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Table->SetCollisionProfileName(TEXT("BlockAll"));

	StatusText3D = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText3D"));
	StatusText3D->SetupAttachment(Root);
	// Lay the readout just above the felt as an angled "overlay" tilted toward the player (~55° back from
	// vertical), instead of a vertical billboard floating overhead.
	StatusText3D->SetRelativeLocation(FVector(0.f, 0.f, TableTopZ + 10.f));
	StatusText3D->SetRelativeRotation(FRotator(55.f, 90.f, 0.f)); // pitch up + face the approach side
	StatusText3D->SetHorizontalAlignment(EHTA_Center);
	StatusText3D->SetWorldSize(16.f);
	StatusText3D->SetTextRenderColor(FColor(255, 240, 180));
	StatusText3D->SetText(FText::FromString(TEXT("Blackjack")));
}

void ABlackjackTable::Play(AFirstPersonCharacter* InPlayer)
{
	if (!InPlayer) { return; }
	APlayerController* PC = Cast<APlayerController>(InPlayer->GetController());
	if (!PC) { return; }

	Player = InPlayer;
	Phase = EPhase::Betting;
	PlayerHand.Reset();
	DealerHand.Reset();
	ClearCardVisuals();
	Status = TEXT("Place your bet and Deal.");

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
	RefreshStatus3D();
	RefreshWidget();
}

int32 ABlackjackTable::GetGold() const
{
	return Player.IsValid() ? Player->GetGold() : 0;
}

int32 ABlackjackTable::DrawCard() const
{
	return FMath::RandRange(1, 13); // infinite shoe (fine for a prototype)
}

int32 ABlackjackTable::HandValue(const TArray<int32>& Hand)
{
	int32 Total = 0, Aces = 0;
	for (int32 Rank : Hand)
	{
		if (Rank == 1) { Aces++; Total += 11; }
		else { Total += FMath::Min(Rank, 10); }
	}
	while (Total > 21 && Aces > 0) { Total -= 10; Aces--; }
	return Total;
}

bool ABlackjackTable::IsBlackjack(const TArray<int32>& Hand)
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
		RefreshStatus3D();
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

	RefreshStatus3D();
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
	}
	RefreshStatus3D();
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

	Phase = EPhase::Resolved;
	RebuildCardVisuals(); // reveal the dealer hole card
	RefreshStatus3D();
	RefreshWidget();
}

void ABlackjackTable::ClearCardVisuals()
{
	for (UTextRenderComponent* L : CardLabels) { if (L) { L->DestroyComponent(); } }
	for (UStaticMeshComponent* M : CardMeshes) { if (M) { M->DestroyComponent(); } }
	CardLabels.Reset();
	CardMeshes.Reset();
}

void ABlackjackTable::SpawnCard(int32 Rank, int32 IndexInRow, bool bDealerRow, bool bFaceDown)
{
	const float Y = bDealerRow ? RowY : -RowY;
	const FVector Loc(IndexInRow * CardSpacingX, Y, CardZ);

	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetupAttachment(Root);
	M->RegisterComponent();
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCastShadow(false);
	if (UStaticMesh* CardMesh = CardMeshOverride ? CardMeshOverride : CubeMesh) { M->SetStaticMesh(CardMesh); }
	M->SetRelativeLocation(Loc);
	M->SetRelativeScale3D(FVector(0.11f, 0.16f, 0.02f)); // flat card ~11 x 16 x 2 cm
	if (UMaterialInstanceDynamic* MID = M->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), bFaceDown ? FLinearColor(0.15f, 0.05f, 0.25f) : FLinearColor(0.95f, 0.95f, 0.92f));
	}
	CardMeshes.Add(M);

	UTextRenderComponent* L = NewObject<UTextRenderComponent>(this);
	L->SetupAttachment(Root);
	L->RegisterComponent();
	L->SetRelativeLocation(Loc + FVector(0.f, 0.f, 2.f));
	L->SetRelativeRotation(FRotator(90.f, 0.f, 0.f)); // lay the text flat, reading from above
	L->SetHorizontalAlignment(EHTA_Center);
	L->SetVerticalAlignment(EVRTA_TextCenter);
	L->SetWorldSize(10.f);
	L->SetTextRenderColor(bFaceDown ? FColor(180, 160, 220) : FColor(20, 20, 20));
	L->SetText(FText::FromString(bFaceDown ? TEXT("?") : CardName(Rank)));
	CardLabels.Add(L);
}

void ABlackjackTable::RebuildCardVisuals()
{
	ClearCardVisuals();

	// Dealer hole card (2nd) is hidden during the player's turn.
	const bool bHideHole = (Phase == EPhase::PlayerTurn);
	for (int32 i = 0; i < DealerHand.Num(); ++i)
	{
		SpawnCard(DealerHand[i], i, /*bDealerRow*/ true, /*bFaceDown*/ bHideHole && i == 1);
	}
	for (int32 i = 0; i < PlayerHand.Num(); ++i)
	{
		SpawnCard(PlayerHand[i], i, /*bDealerRow*/ false, /*bFaceDown*/ false);
	}
}

void ABlackjackTable::RefreshStatus3D()
{
	if (!StatusText3D) { return; }
	FString Line;
	if (PlayerHand.Num() == 0)
	{
		Line = TEXT("BLACKJACK\nPlace your bet");
	}
	else
	{
		const bool bReveal = (Phase != EPhase::PlayerTurn);
		const FString Dealer = bReveal ? FString::Printf(TEXT("Dealer %d"), HandValue(DealerHand)) : TEXT("Dealer ?");
		Line = FString::Printf(TEXT("%s\nYou %d\n%s"), *Dealer, HandValue(PlayerHand), *Status);
	}
	StatusText3D->SetText(FText::FromString(Line));
}

void ABlackjackTable::RefreshWidget()
{
	if (Widget) { Widget->RefreshFromTable(); }
}

void ABlackjackTable::Leave()
{
	if (Widget) { Widget->CloseBar(); }
	ClearCardVisuals();
	Phase = EPhase::Betting;
	PlayerHand.Reset();
	DealerHand.Reset();
	RefreshStatus3D();
}
