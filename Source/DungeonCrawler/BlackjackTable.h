#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlackjackTable.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AFirstPersonCharacter;
class UBlackjackWidget;

/**
 * A 3D blackjack station for the town. The table + dealt cards are real world geometry (graybox: a felt
 * slab + thin card meshes with the rank printed on them); a floating label shows hand values + the
 * result. The rules engine lives here; a slim on-screen control bar (UBlackjackWidget) drives the
 * actions. Bets/payouts use the player's gold. Swap the table/card meshes for art later.
 */
UCLASS()
class DUNGEONCRAWLER_API ABlackjackTable : public AActor
{
	GENERATED_BODY()

public:
	ABlackjackTable();

	/** Opens the control bar for the interacting player and resets to the betting phase. */
	void Play(AFirstPersonCharacter* Player);

	// --- Actions (called by the control widget) ---
	void Deal();
	void Hit();
	void Stand();
	void BetUp();
	void BetDown();
	void Leave();

	// --- State the control widget reads ---
	int32 GetBet() const { return Bet; }
	int32 GetGold() const;
	bool CanBet() const { return Phase != EPhase::PlayerTurn; } // betting / resolved
	bool CanHitStand() const { return Phase == EPhase::PlayerTurn; }
	FString GetStatusLine() const { return Status; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<USceneComponent> Root;

	/** Felt slab — blocks the player + is the interact line-trace target. */
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UStaticMeshComponent> Table;

	/** Floating label above the table (hand values / result). */
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UTextRenderComponent> StatusText3D;

	/** Finished-mesh swap-in points (null = keep the graybox). */
	UPROPERTY(EditAnywhere, Category = "Blackjack|Meshes")
	TObjectPtr<UStaticMesh> TableMeshOverride;

	UPROPERTY(EditAnywhere, Category = "Blackjack|Meshes")
	TObjectPtr<UStaticMesh> CardMeshOverride;

private:
	enum class EPhase : uint8 { Betting, PlayerTurn, Resolved };

	int32 DrawCard() const;
	static int32 HandValue(const TArray<int32>& Hand);
	static bool IsBlackjack(const TArray<int32>& Hand);
	static FString CardName(int32 Rank);

	void DealerPlayAndResolve();
	void RebuildCardVisuals(); // (re)spawn the 3D cards for both hands
	void ClearCardVisuals();
	void SpawnCard(int32 Rank, int32 IndexInRow, bool bDealerRow, bool bFaceDown);
	void RefreshStatus3D();
	void RefreshWidget();

	TArray<int32> PlayerHand;
	TArray<int32> DealerHand;
	int32 Bet = 25;
	EPhase Phase = EPhase::Betting;
	FString Status;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CardMeshes;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> CardLabels;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TWeakObjectPtr<AFirstPersonCharacter> Player;
	UPROPERTY() TObjectPtr<UBlackjackWidget> Widget;
};
