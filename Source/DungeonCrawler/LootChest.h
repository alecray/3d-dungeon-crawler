#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootChest.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UInventoryComponent;

/**
 * A graybox lootable chest. Interacting (E) opens it: it rolls loot into its own inventory and flips
 * its lid. The chest's inventory is shown as a draggable grid (same widget as the player inventory) —
 * the player loots by dragging items from the chest grid into their own.
 */
UCLASS()
class DUNGEONCRAWLER_API ALootChest : public AActor
{
	GENERATED_BODY()

public:
	ALootChest();

	/** Opens the chest (once): rolls loot and flips the lid. Safe to call again (no-op once opened). */
	void Open();

	bool IsOpened() const { return bOpened; }

	UInventoryComponent* GetInventory() const { return ChestInventory; }

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

	UPROPERTY(VisibleAnywhere, Category = "Chest")
	TObjectPtr<UInventoryComponent> ChestInventory;

	/** Number of item drops rolled when opened. */
	UPROPERTY(EditAnywhere, Category = "Chest")
	int32 MinDrops = 2;

	UPROPERTY(EditAnywhere, Category = "Chest")
	int32 MaxDrops = 5;

private:
	void RollLoot();
	bool bOpened = false;
};
