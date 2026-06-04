#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbientDust.generated.h"

class UStaticMeshComponent;

/**
 * Cheap ambient "dust motes": a fixed pool of tiny specks that drift slowly and are recycled around the
 * player as they wander off, so the count (and cost) stays constant no matter the dungeon size. They
 * catch the torch light and read as floating dust. Code-driven, no art/Niagara. Spawned once per level.
 */
UCLASS()
class DUNGEONCRAWLER_API AAmbientDust : public AActor
{
	GENERATED_BODY()

public:
	AAmbientDust();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Number of motes kept alive (the whole budget). */
	UPROPERTY(EditAnywhere, Category = "Dust") int32 MoteCount = 28;
	/** Motes live within this radius of the player; beyond it they recycle to a new spot near the player. */
	UPROPERTY(EditAnywhere, Category = "Dust") float Radius = 850.f;
	UPROPERTY(EditAnywhere, Category = "Dust") float DriftSpeed = 16.f;
	UPROPERTY(EditAnywhere, Category = "Dust") float MoteScale = 0.035f;

private:
	FVector PlayerLoc() const;
	FVector RandomSpotNear(const FVector& Center) const;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Motes;
	TArray<FVector> Velocities;
};
