#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "LootChest.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UInventoryComponent;
class ALootChest;

/** Fired when the chest's contents change (item taken), so the loot pane can refresh. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChestContentsChanged, ALootChest*);

/**
 * A graybox lootable chest. The player interacts (E) to open it: it rolls loot into its own
 * container and flips its lid. A loot pane then shows the contents and the player picks what to take
 * (per-item or Take All) — nothing is auto-collected.
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
	bool IsEmpty() const;

	const TArray<FInventorySlot>& GetContents() const { return Contents; }

	/** Moves the stack at Index into the given inventory (as much as fits), then clears it. */
	void TakeItem(int32 Index, UInventoryComponent* Into);

	/** Takes everything that fits. */
	void TakeAll(UInventoryComponent* Into);

	FOnChestContentsChanged OnContentsChanged;

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

	/** Number of item drops rolled when opened. */
	UPROPERTY(EditAnywhere, Category = "Chest")
	int32 MinDrops = 2;

	UPROPERTY(EditAnywhere, Category = "Chest")
	int32 MaxDrops = 5;

private:
	void RollLoot();

	UPROPERTY()
	TArray<FInventorySlot> Contents;

	bool bOpened = false;
};
