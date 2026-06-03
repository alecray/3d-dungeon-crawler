#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootChest.generated.h"

class USkeletalMeshComponent;
class UBoxComponent;
class UInventoryComponent;
class UAnimSequence;

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

	/** Opens the chest: rolls loot the first time, and plays the open animation. */
	void Open();

	/** Plays the close animation (visual only — loot is unaffected). Called when the loot menu closes. */
	void CloseLid();

	bool IsOpened() const { return bOpened; }

	/** True if the viewer is within the chest's front-facing arc (so it can only be opened from the front). */
	bool IsViewerInFront(const FVector& ViewerLocation) const;

	UInventoryComponent* GetInventory() const { return ChestInventory; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void ResolveMeshes();

	/** Box collider (root) sized to the mesh — gives the skeletal chest collision + an interact target. */
	UPROPERTY(VisibleAnywhere, Category = "Chest")
	TObjectPtr<UBoxComponent> Collision;

	/** Skeletal chest mesh; plays the open/close animations. */
	UPROPERTY(VisibleAnywhere, Category = "Chest")
	TObjectPtr<USkeletalMeshComponent> ChestMesh;

	UPROPERTY() TObjectPtr<UAnimSequence> OpenAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> CloseAnim;

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
