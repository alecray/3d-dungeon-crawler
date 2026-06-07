#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FishingComponent.generated.h"

class USkeletalMesh;
class UAnimSequence;
class AFirstPersonCharacter;

/** Which fishing animation the held rod is playing (drives the rod mesh's single-node anim). */
UENUM(BlueprintType)
enum class EFishingPose : uint8
{
	Cast,    // throwing the line out
	Idle,    // line in the water, waiting for a bite
	Tension, // fish on the line, deciding catch vs fail
	Reel     // reeling in (success or fail)
};

/**
 * Owns the player's fishing state, extracted from AFirstPersonCharacter: swaps the held weapon mesh
 * for a fishing rod, plays the rod animations, tracks the auto-fading on-screen status line, and
 * reports whether a rod is carried. Driven by AFishingHole; the HUD polls GetFishingStatus().
 */
UCLASS(ClassGroup = (DungeonCrawler), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UFishingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFishingComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** True while fishing — the rod is shown in hand in place of the equipped weapon. */
	bool IsFishing() const { return bFishing; }
	/** True if a Fishing Rod is in the owner's inventory (required to cast a line). */
	bool HasFishingRod() const;
	/** Swap the held weapon to the fishing rod and enter the fishing pose. */
	void BeginFishing();
	/** Play a rod animation: Cast/Reel play once; Idle/Tension loop. No-ops if the anim isn't authored yet. */
	void PlayFishingPose(EFishingPose Pose);
	/** Leave fishing: restore the equipped weapon (the status line fades on its own afterward). */
	void EndFishing();
	/** Set the on-screen fishing status line (shown by the HUD); it auto-fades once not fishing. */
	void SetFishingStatus(const FString& Status);
	const FString& GetFishingStatus() const { return FishingStatus; }

protected:
	/** Fishing rod skeletal mesh, shown in hand while fishing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
	TSoftObjectPtr<USkeletalMesh> FishingRodSkeletalAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Tools/SK_Fishing_Rod.SK_Fishing_Rod")));

	/** Fishing animations (null until authored — they no-op so the flow still works). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
	TSoftObjectPtr<UAnimSequence> FishingCastAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Tools/A_Fishing_Rod_Cast.A_Fishing_Rod_Cast")));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
	TSoftObjectPtr<UAnimSequence> FishingIdleAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Tools/A_Fishing_Rod_Idle.A_Fishing_Rod_Idle")));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
	TSoftObjectPtr<UAnimSequence> FishingTensionAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Tools/A_Fishing_Rod_Tension.A_Fishing_Rod_Tension")));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
	TSoftObjectPtr<UAnimSequence> FishingReelAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Tools/A_Fishing_Rod_Reel_In.A_Fishing_Rod_Reel_In")));

	/** Seconds a transient status line stays before fading (when not actively fishing). */
	UPROPERTY(EditAnywhere, Category = "Fishing")
	float FishingStatusDuration = 4.5f;

private:
	/** Owning player, cached in BeginPlay (used to reach the held weapon mesh + inventory). */
	UPROPERTY()
	TObjectPtr<AFirstPersonCharacter> OwnerChar;

	bool bFishing = false;          // rod shown in hand instead of the weapon
	FString FishingStatus;          // current on-screen status line (HUD reads it)
	float FishingStatusLeft = 0.f;  // seconds the status line stays before fading
};
