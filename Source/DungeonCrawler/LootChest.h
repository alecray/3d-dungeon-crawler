#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootChest.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/**
 * A graybox lootable chest. The player interacts (E) to open it once: it rolls a few items from the
 * item database into the player's inventory and flips its lid open. Built from cubes / the crate mesh.
 */
UCLASS()
class DUNGEONCRAWLER_API ALootChest : public AActor
{
	GENERATED_BODY()

public:
	ALootChest();

	/** Opens the chest (once): rolls loot into the interactor's inventory and flips the lid. */
	void Interact(AActor* Interactor);

	bool IsOpened() const { return bOpened; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void ResolveMeshes();

	UPROPERTY(VisibleAnywhere, Category = "Chest")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Lid rotates around this pivot (placed at the back-top edge). */
	UPROPERTY(VisibleAnywhere, Category = "Chest")
	TObjectPtr<USceneComponent> LidPivot;

	UPROPERTY(VisibleAnywhere, Category = "Chest")
	TObjectPtr<UStaticMeshComponent> LidMesh;

	/** Number of item drops when opened. */
	UPROPERTY(EditAnywhere, Category = "Chest")
	int32 MinDrops = 1;

	UPROPERTY(EditAnywhere, Category = "Chest")
	int32 MaxDrops = 3;

private:
	bool bOpened = false;
};
