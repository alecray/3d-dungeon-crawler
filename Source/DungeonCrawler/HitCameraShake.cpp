#include "HitCameraShake.h"

static constexpr float HitShakeDuration = 0.16f;

UHitCameraShake::UHitCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetRootShakePattern(ObjectInitializer.CreateDefaultSubobject<UHitShakePattern>(this, TEXT("Pattern")));
}

void UHitShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(HitShakeDuration);
	OutInfo.BlendIn = 0.01f;
	OutInfo.BlendOut = 0.1f;
}

void UHitShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;
	const float T = Elapsed;
	const float Damp = FMath::Clamp(1.f - Elapsed / HitShakeDuration, 0.f, 1.f);

	// Quick punch: a pitch kick with a little roll, fading fast.
	OutResult.Rotation = FRotator(
		FMath::Sin(T * 2.f * PI * 30.f)        * 0.9f,   // pitch
		FMath::Sin(T * 2.f * PI * 24.f + 1.1f) * 0.4f,   // yaw
		FMath::Sin(T * 2.f * PI * 27.f + 0.5f) * 0.7f) * Damp; // roll
	OutResult.Location = FVector(0.f, 0.f, FMath::Sin(T * 2.f * PI * 32.f) * 1.5f) * Damp;
	OutResult.FOV = 0.f;
}
