#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlackjackTable.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UTextRenderComponent;
class UTexture2D;
class UMaterialInterface;
class AFirstPersonCharacter;
class UBlackjackWidget;

/** One dealt card. Suit is cosmetic (infinite shoe); rank drives hand value. */
struct FBJCard
{
	int32 Rank = 1; // 1=Ace .. 13=King
	int32 Suit = 0; // 0=clubs 1=diamonds 2=hearts 3=spades
};

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

	virtual void BeginPlay() override;

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

	/** Table mesh (visual only — collision lives on the Collision box so it's independent of the art's
	    authored collision / pivot). */
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UStaticMeshComponent> Table;

	/** Blocks the player + is the interact line-trace target; sized to the real table bounds in BeginPlay
	    so it stays correct regardless of the mesh's origin. */
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UBoxComponent> Collision;

	/** Unlit/masked material that shows a card face (CardTex param); cards are textured planes. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> CardMaterial;

	/** Static "Blackjack" sign standing at the back of the table (just a label, not the live status). */
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UTextRenderComponent> TitleText;

	/** World-space hand-value labels beside each row: "Dealer N" by the dealer cards, "You N" by yours. */
	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UTextRenderComponent> DealerValueText;

	UPROPERTY(VisibleAnywhere, Category = "Blackjack")
	TObjectPtr<UTextRenderComponent> PlayerValueText;

	/** Finished-mesh swap-in points (null = keep the graybox). */
	UPROPERTY(EditAnywhere, Category = "Blackjack|Meshes")
	TObjectPtr<UStaticMesh> TableMeshOverride;

	UPROPERTY(EditAnywhere, Category = "Blackjack|Meshes")
	TObjectPtr<UStaticMesh> CardMeshOverride;

private:
	enum class EPhase : uint8 { Betting, PlayerTurn, Resolved };

	FBJCard DrawCard() const;
	static int32 HandValue(const TArray<FBJCard>& Hand);
	static bool IsBlackjack(const TArray<FBJCard>& Hand);
	static FString CardName(int32 Rank);
	static FString CardTextureName(const FBJCard& Card); // e.g. "hearts_A"

	/** Decode a card PNG from Content/Images/Cards into a transient texture (cached by file stem). */
	UTexture2D* GetCardTexture(const FString& Stem);

	void DealerPlayAndResolve();
	void RebuildCardVisuals(); // (re)spawn the 3D cards for both hands
	void ClearCardVisuals();
	void SpawnCard(const FBJCard& Card, int32 IndexInRow, int32 RowCount, bool bDealerRow, bool bFaceDown);
	void RefreshWidget();
	void RefreshValueLabels(); // updates/clears the 3D "Dealer N" / "You N" labels by the cards

	/** Table mesh bounds in the actor's local space (accounts for the mesh's scale/rotation/offset),
	 *  so cards can be placed on the real felt regardless of which art mesh is used. */
	FBox TableBoundsActor() const;
	float TableSurfaceZ() const { return TableBoundsActor().Max.Z; }

	TArray<FBJCard> PlayerHand;
	TArray<FBJCard> DealerHand;
	int32 Bet = 25;
	EPhase Phase = EPhase::Betting;
	FString Status;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CardMeshes;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> PlaneMesh;
	UPROPERTY() TMap<FString, TObjectPtr<UTexture2D>> TextureCache;
	UPROPERTY() TWeakObjectPtr<AFirstPersonCharacter> Player;
	UPROPERTY() TObjectPtr<UBlackjackWidget> Widget;
};
