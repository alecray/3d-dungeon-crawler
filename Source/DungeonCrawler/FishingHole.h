#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingHole.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AFirstPersonCharacter;

/**
 * A simple 3D fishing spot for the town. Interact (E) to cast a line; after a random wait the bobber
 * shakes (a bite) — interact again within the short window to land a random fish (added to inventory).
 * Miss the window and it gets away. Graybox water + bobber + caught-fish meshes have swap-in points for
 * real models. The catch table is weighted toward small fry; a Golden Carp is the rare prize.
 */
UCLASS()
class DUNGEONCRAWLER_API AFishingHole : public AActor
{
	GENERATED_BODY()

public:
	AFishingHole();

	virtual void Tick(float DeltaSeconds) override;

	/** Cast, reel-early, or land the catch depending on the current state. */
	void Interact(AFirstPersonCharacter* Player);

	/** Context prompt shown by the HUD: "Cast a line" / "Reel in" / "REEL IN!". */
	FString GetPrompt() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UStaticMeshComponent> HoleMesh; // real art (SM_Fishing_Hole)
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UStaticMeshComponent> Water;   // interact target (invisible once art is in)
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UStaticMeshComponent> Bobber;  // on the water while cast
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UStaticMeshComponent> CaughtFish; // shown briefly on a catch
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UTextRenderComponent> StatusText3D;

	UPROPERTY(EditAnywhere, Category = "Fishing|Meshes") TObjectPtr<UStaticMesh> WaterMeshOverride;
	UPROPERTY(EditAnywhere, Category = "Fishing|Meshes") TObjectPtr<UStaticMesh> BobberMeshOverride;
	UPROPERTY(EditAnywhere, Category = "Fishing|Meshes") TObjectPtr<UStaticMesh> FishMeshOverride;

	UPROPERTY(EditAnywhere, Category = "Fishing") float MinWait = 2.5f;
	UPROPERTY(EditAnywhere, Category = "Fishing") float MaxWait = 7.f;
	UPROPERTY(EditAnywhere, Category = "Fishing") float BiteWindow = 1.3f;

private:
	enum class EState : uint8 { Idle, Waiting, Biting };

	void StartBite();
	void SetStatus(const FString& S);
	FName RollFish() const;
	void ShowCatch(FName FishId);

	EState State = EState::Idle;
	float BiteLeft = 0.f;      // remaining window while Biting
	float BobPhase = 0.f;
	float CatchDisplayLeft = 0.f;
	float WaterTopZ = 0.f;

	FTimerHandle BiteTimer;
	UPROPERTY() TWeakObjectPtr<AFirstPersonCharacter> Angler;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
};
