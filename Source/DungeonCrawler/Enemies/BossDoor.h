#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossDoor.generated.h"

class UStaticMeshComponent;

/**
 * A graybox barrier that seals a boss-room doorway for the duration of the fight. Built from a single
 * cube scaled to fill the opening; it rises up out of the floor when sealed and sinks back down (then
 * destroys) when opened. Spawned by ABossArena. Swap the cube for a portcullis/gate mesh later.
 */
UCLASS()
class DUNGEONCRAWLER_API ABossDoor : public AActor
{
	GENERATED_BODY()

public:
	ABossDoor();

	/** Sizes the barrier to fill a doorway (full extents in cm). Call right after spawning. */
	void Init(const FVector& SizeCm);

	/** Raises the barrier to block the doorway. */
	void Seal();

	/** Lowers the barrier back into the floor and destroys it once hidden. */
	void Open();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "BossDoor")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	float Height = 320.f;   // travel distance (full barrier height)
	float Target = 0.f;     // 0 = hidden below floor, 1 = fully raised
	float Current = 0.f;
	bool bOpening = false;  // when true and fully lowered, destroy

	void ApplyOffset();
};
