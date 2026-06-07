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
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Loads M_PondWater (runtime, not in the CDO so the asset stays editable by the build script) and
	    applies it to the Water box (and the HoleMesh water slot). Safe to call repeatedly. */
	void ApplyWaterMaterial();

	/** Sizes/positions the visible Water box from WaterExtent (X/Y only — the surface height is kept) and
	    makes it the lone fishing target by keeping HoleMesh out of the interact (Visibility) trace. */
	void UpdateWaterBox();

	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UStaticMeshComponent> HoleMesh; // real art (SM_Fishing_Hole)
	UPROPERTY(VisibleAnywhere, Category = "Fishing") TObjectPtr<UStaticMeshComponent> Water;    // the visible water surface + fishing target

	/** Footprint of the oval water surface, in cm (X = width, Y = length — set them unequal for an ellipse).
	    Tune in the Details panel to fill the basin out to the rocks; OnConstruction re-sizes it live. */
	UPROPERTY(EditAnywhere, Category = "Fishing|Water") FVector2D WaterExtent = FVector2D(1050.f, 1050.f);

	/** Local height (cm) of the water surface — raise it to bring the water up toward the rim. The bobber and
	    cast/reel logic follow this. */
	UPROPERTY(EditAnywhere, Category = "Fishing|Water") float WaterHeight = 30.f;

	/** Tint of the water. Drives M_PondWater's shallow color; the deep color is a darker shade of it. */
	UPROPERTY(EditAnywhere, Category = "Fishing|Water") FLinearColor WaterColor = FLinearColor(0.10f, 0.45f, 0.55f, 1.0f);

	/** Which SM_Fishing_Hole material slot also gets M_PondWater (the model's own water face, under the box).
	    The model has 2 slots [0]=Material, [1]=Material_0 — flip this if the wrong face turns to water. */
	UPROPERTY(EditAnywhere, Category = "Fishing|Meshes") int32 WaterMaterialSlot = 1;
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
