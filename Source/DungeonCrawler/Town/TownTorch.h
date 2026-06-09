#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TownTorch.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class ADayNightCycle;

/**
 * Decorative standing torch for the town area. Uses SM_Standing_Torch as the base mesh with a small
 * emissive flame cube at the top. The flame and point light automatically come on at dusk and go off
 * at dawn by reading the town's ADayNightCycle time. During daylight the torch is fully dark.
 */
UCLASS()
class DUNGEONCRAWLER_API ATownTorch : public AActor
{
	GENERATED_BODY()

public:
	ATownTorch();

	virtual void Tick(float DeltaSeconds) override;

	/** Height above the actor origin where the flame cube and light are placed.
	 *  Tune this in the Details panel to align with the torch bowl on the mesh. */
	UPROPERTY(EditAnywhere, Category = "TownTorch", meta = (ClampMin = "50"))
	float FlameHeight = 175.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "TownTorch")
	TObjectPtr<USceneComponent> Root;

	/** SM_Standing_Torch mesh — blocks player movement and the interact trace. */
	UPROPERTY(VisibleAnywhere, Category = "TownTorch")
	TObjectPtr<UStaticMeshComponent> TorchMesh;

	/** Small emissive cube standing in for the flame; fades out during the day. */
	UPROPERTY(VisibleAnywhere, Category = "TownTorch")
	TObjectPtr<UStaticMeshComponent> Flame;

	UPROPERTY(VisibleAnywhere, Category = "TownTorch")
	TObjectPtr<UPointLightComponent> Light;

private:
	UPROPERTY() TObjectPtr<ADayNightCycle> DayCycle;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMID;

	float FlickerPhase = 0.f;

	static constexpr float BaseLightIntensity = 2200.f;
	static constexpr FLinearColor FlameColor  = FLinearColor(1.4f, 0.55f, 0.12f);
};
