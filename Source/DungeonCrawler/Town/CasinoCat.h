#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "CasinoCat.generated.h"

class USkeletalMeshComponent;

/**
 * Cosmetic ambient prop: a Casino Cat skeletal mesh that continuously plays weighted-random idle
 * animations. Each time the current clip ends the actor re-rolls among Idle_0/1/2 by weight (default
 * 50 / 30 / 20 %) and starts the next one, so the cat never freezes and the distribution holds
 * across time.
 *
 * Place this actor in L_Town at the location where the plain SK_Casino_Cat static/skeletal actor was
 * (the raw mesh actor has no anim driver, so swap it for this). No collision — purely cosmetic.
 */
UCLASS()
class DUNGEONCRAWLER_API ACasinoCat : public AActor
{
	GENERATED_BODY()

public:
	ACasinoCat();

protected:
	virtual void BeginPlay() override;

	// ---- Skeletal mesh ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Casino Cat")
	TObjectPtr<USkeletalMeshComponent> CatMesh;

	// ---- Asset refs (soft so they're not forced-loaded by the constructor) ----
	UPROPERTY(EditAnywhere, Category = "Casino Cat|Assets")
	TSoftObjectPtr<USkeletalMesh> SkeletalMeshAsset;

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations")
	TSoftObjectPtr<UAnimSequence> Idle0;

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations")
	TSoftObjectPtr<UAnimSequence> Idle1;

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations")
	TSoftObjectPtr<UAnimSequence> Idle2;

	// ---- Weights (must sum to 1 for a clean probability, but any positive values work; they are
	//      normalized at runtime so the Details panel is forgiving of arbitrary values) ----
	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight0 = 0.50f;  // Idle_0 — most common; cat sits quietly

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight1 = 0.30f;  // Idle_1 — occasional variant

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight2 = 0.20f;  // Idle_2 — rarest variant

private:
	/** Roll weighted-random among non-null clips, play it once, then set a timer to call this again. */
	void PlayWeightedIdle();

	// Loaded clip pointers (filled in BeginPlay; UPROPERTY to prevent GC between timer fires).
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle0;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle1;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle2;

	FTimerHandle IdleTimerHandle;
};
