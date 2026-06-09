#pragma once

#include "CoreMinimal.h"
#include "MonsterCharacter.h"
#include "FrogMonster.generated.h"

/**
 * Frog enemy: charges by hopping rather than walking. Stands still briefly between hops,
 * then winds up and launches toward the player with a ballistic arc. A_Frog_Walk drives
 * the full hop cycle; the idle animation plays during the pause between hops.
 *
 * Movement timing guide (30 fps, A_Frog_Walk = 30 frames):
 *   Frames  1– 8  wind-up on the ground (crouch/compress).
 *   Frame   9     LaunchCharacter fires — frog leaves the ground.
 *   Frames  9–30  airborne tuck (22 frames ≈ 0.73 s at HopUpSpeed 360 cm/s).
 *   HopWindupDuration should equal the wind-up frame count / fps (8/30 ≈ 0.267 s).
 */
UCLASS()
class DUNGEONCRAWLER_API AFrogMonster : public AMonsterCharacter
{
	GENERATED_BODY()

public:
	AFrogMonster();

protected:
	virtual bool TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist) override;

	/**
	 * Suppress run-locomotion requests from the base class during the idle phase between hops
	 * so the frog stands still instead of playing the hop animation while waiting.
	 */
	virtual void SetLocomotion(bool bMoving) override;
	virtual void PlayAttackAnim() override;

	/** Pause between hops (seconds). Frog stands idle for this long before starting a wind-up. */
	UPROPERTY(EditAnywhere, Category = "Frog")
	float HopCooldown = 1.1f;

	/**
	 * Duration of the wind-up phase (seconds). A_Frog_Walk plays from frame 1 during this
	 * window; LaunchCharacter fires when it expires. 8 frames at 30fps = 0.267s.
	 */
	UPROPERTY(EditAnywhere, Category = "Frog")
	float HopWindupDuration = 0.267f;

	/** Forward velocity applied on each hop launch (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Frog")
	float HopForwardSpeed = 520.f;

	/**
	 * Upward velocity applied on each hop launch (cm/s).
	 * Airborne time = 2 * HopUpSpeed / 981. At 360 cm/s → 0.73s → 22 frames at 30fps.
	 * Tune to match the airborne section of A_Frog_Walk.
	 */
	UPROPERTY(EditAnywhere, Category = "Frog")
	float HopUpSpeed = 360.f;

private:
	bool  bIsHopping      = false;
	bool  bIsWindingUp    = false;
	float HopCooldownLeft = 0.f;
	float WindupLeft      = 0.f;
};
