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

	/** Play the one-shot "deal" animation once (interrupting the idle loop), then resume looping idles.
	 *  No-op if no deal clip is set/loaded. Called by ABlackjackTable when a hand is dealt. */
	void PlayDeal();

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

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations")
	TSoftObjectPtr<UAnimSequence> Idle3;

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations")
	TSoftObjectPtr<UAnimSequence> Idle4;

	/** One-shot dealer "deal" animation, played by PlayDeal() (not part of the idle roll). */
	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations")
	TSoftObjectPtr<UAnimSequence> DealAnim;

	// ---- Weights (any positive values work; they are normalized at runtime, so the Details panel is
	//      forgiving — tweak these on the placed actor without recompiling) ----
	// Idle_0 is 50%; the other four idles split the remaining 50% evenly (12.5% each).
	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight0 = 0.50f;   // Idle_0 — half the time

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight1 = 0.125f;  // Idle_1

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight2 = 0.125f;  // Idle_2

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight3 = 0.125f;  // Idle_3

	UPROPERTY(EditAnywhere, Category = "Casino Cat|Animations", meta = (ClampMin = "0.0"))
	float Weight4 = 0.125f;  // Idle_4

private:
	/** Roll weighted-random among non-null clips, play it once, then set a timer to call this again. */
	void PlayWeightedIdle();

	// Loaded clip pointers (filled in BeginPlay; UPROPERTY to prevent GC between timer fires).
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle0;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle1;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle2;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle3;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle4;
	UPROPERTY() TObjectPtr<UAnimSequence> LoadedDeal;

	FTimerHandle IdleTimerHandle;
};
