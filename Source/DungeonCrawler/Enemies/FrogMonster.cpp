#include "FrogMonster.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Math/UnrealMathUtility.h"

AFrogMonster::AFrogMonster()
{
}

// ---------------------------------------------------------------------------
// Locomotion override — suppress run-anim requests during the idle phase
// ---------------------------------------------------------------------------

void AFrogMonster::SetLocomotion(bool bMoving)
{
	// During idle and wind-up phases the frog manages its own animation (idle → A_Frog_Walk
	// started manually at wind-up start). Suppress run requests from the base class so the
	// animation isn't restarted every frame. Only forward when we're actively airborne.
	if (bMoving && !bIsHopping)
	{
		return;
	}
	Super::SetLocomotion(bMoving);
}

// ---------------------------------------------------------------------------
// Attack override — no attacks while airborne or winding up
// ---------------------------------------------------------------------------

void AFrogMonster::PlayAttackAnim()
{
	// Don't interrupt A_Frog_Walk mid-hop or mid-windup with an attack animation.
	if (bIsHopping || bIsWindingUp)
	{
		return;
	}
	Super::PlayAttackAnim();
}

// ---------------------------------------------------------------------------
// Custom chase — hop-based movement
// ---------------------------------------------------------------------------

bool AFrogMonster::TickCustomChase(float DeltaSeconds, APawn* Player, const FVector& DirToPlayer, float Dist)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return false;
	}

	// --- Phase: AIRBORNE ---
	if (bIsHopping)
	{
		// Wait until physics brings the frog back to the ground.
		if (!Move->IsFalling())
		{
			bIsHopping      = false;
			HopCooldownLeft = HopCooldown;
			Super::SetLocomotion(false); // snap to idle on landing
		}
		// While airborne let physics drive position — no AddMovementInput needed.
		return true;
	}

	// --- Phase: WIND-UP ---
	if (bIsWindingUp)
	{
		WindupLeft -= DeltaSeconds;
		if (WindupLeft <= 0.f)
		{
			// Wind-up complete — launch. Animation is already playing from when wind-up began;
			// don't call SetLocomotion again or it would restart A_Frog_Walk mid-frame.
			bIsWindingUp = false;
			bIsHopping   = true;
			LaunchCharacter(
				DirToPlayer * HopForwardSpeed + FVector(0.f, 0.f, HopUpSpeed),
				/*bXYOverride*/ true,
				/*bZOverride*/  true);
		}
		return true;
	}

	// --- Phase: IDLE (cooling down between hops) ---
	// Slowly face the player while waiting.
	const FRotator FacePlayer(0.f, DirToPlayer.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), FacePlayer, DeltaSeconds, 5.f));

	HopCooldownLeft -= DeltaSeconds;
	if (HopCooldownLeft > 0.f)
	{
		return true; // SetLocomotion override keeps the idle anim playing
	}

	// Cooldown expired — begin wind-up. Snap to face the player precisely, start A_Frog_Walk
	// from frame 1 (wind-up frames), and count down before the actual launch.
	SetActorRotation(FacePlayer);
	bIsWindingUp = true;
	WindupLeft   = HopWindupDuration;
	Super::SetLocomotion(true); // starts A_Frog_Walk — frames 1-8 are the crouch/compress wind-up
	return true;
}
