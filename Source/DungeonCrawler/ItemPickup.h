#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

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

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	UPROPERTY() FName ItemId;
	UPROPERTY() int32 Count = 1;
	float Spin = 0.f;
};
