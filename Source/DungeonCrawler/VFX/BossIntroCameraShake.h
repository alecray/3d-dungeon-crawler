#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "BossIntroCameraShake.generated.h"

/**
 * A short, punchy camera rattle for a boss's intro roar. Implemented as a self-contained shake pattern
 * (damped sine oscillation on location + rotation) so it needs no asset and no extra plugin module —
 * just the Engine's UCameraShakeBase/UCameraShakePattern. Started via ClientStartCameraShake.
 */
UCLASS()
class DUNGEONCRAWLER_API UBossIntroShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

protected:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

private:
	float Elapsed = 0.f;
};

UCLASS()
class DUNGEONCRAWLER_API UBossIntroCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UBossIntroCameraShake(const FObjectInitializer& ObjectInitializer);
};
