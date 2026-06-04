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
