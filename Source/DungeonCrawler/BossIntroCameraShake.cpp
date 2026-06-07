#include "BossIntroCameraShake.h"

static constexpr float ShakeDuration = 1.2f;

UBossIntroCameraShake::UBossIntroCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetRootShakePattern(ObjectInitializer.CreateDefaultSubobject<UBossIntroShakePattern>(this, TEXT("Pattern")));
}

void UBossIntroShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(ShakeDuration);
	OutInfo.BlendIn = 0.1f;
	OutInfo.BlendOut = 0.45f;
}

void UBossIntroShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;
	const float T = Elapsed;
	const float Damp = FMath::Clamp(1.f - Elapsed / ShakeDuration, 0.f, 1.f); // taper off over the duration

	// Sum of fast sine waves on each axis. The per-axis frequencies (16-22 Hz) are deliberately mismatched
	// and phase-offset so the motion never repeats or syncs into an obvious wobble — it reads as a rumble.
	// Amplitudes are in cm (location) / degrees (rotation): vertical shake (Z, roll) is biggest for impact.
	OutResult.Location = FVector(
		FMath::Sin(T * 2.f * PI * 18.f)         * 3.f,
		FMath::Sin(T * 2.f * PI * 16.f + 1.3f)  * 3.f,
		FMath::Sin(T * 2.f * PI * 20.f + 0.7f)  * 5.f) * Damp;

	OutResult.Rotation = FRotator(
		FMath::Sin(T * 2.f * PI * 22.f)         * 1.5f,   // pitch
		FMath::Sin(T * 2.f * PI * 19.f + 0.9f)  * 1.5f,   // yaw
		FMath::Sin(T * 2.f * PI * 17.f + 2.1f)  * 2.5f) * Damp; // roll

	OutResult.FOV = 0.f;
}
