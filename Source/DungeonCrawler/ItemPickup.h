#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;

/** A dropped item in the world (graybox). Auto-picks-up when a pawn with an inventory overlaps it. */
UCLASS()
class DUNGEONCRAWLER_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	virtual void Tick(float DeltaSeconds) override;

	/** Set what this pickup grants. */
	void Configure(FName InItemId, int32 InCount);

	/** Adds the item to the pawn's inventory; destroys the pickup if fully collected. Returns true if so. */
	bool Collect(APawn* ByPawn);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Used when the item's icon is a skeletal mesh (e.g. potions, weapons); hidden otherwise.
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USkeletalMeshComponent> SkelMesh;

private:
	/** The component currently displaying the item (Mesh or SkelMesh), spun in Tick. */
	UPROPERTY() TObjectPtr<USceneComponent> Display;

	UPROPERTY() FName ItemId;
	UPROPERTY() int32 Count = 1;
	float Spin = 0.f;
};
