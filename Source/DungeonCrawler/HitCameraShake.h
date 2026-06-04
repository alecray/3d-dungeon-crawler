#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "HitCameraShake.generated.h"

/**
 * A short, snappy camera kick for landing/firing an attack — a quick damped pitch/roll punch. Pure
 * code (a self-contained UCameraShakePattern), no asset needed. Started via ClientStartCameraShake;
 * scale it down for ranged recoil, up for heavy melee impacts.
 */
UCLASS()
class DUNGEONCRAWLER_API UHitShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

protected:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

private:
	float Elapsed = 0.f;
};

UCLASS()
class DUNGEONCRAWLER_API UHitCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UHitCameraShake(const FObjectInitializer& ObjectInitializer);
};
