#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycle.generated.h"

class ADirectionalLight;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Town-only day/night cycle. Rotates the level's existing sun over a configurable day length and lerps
 * sun/moon color + intensity, the sky-light floor, fog color, and a star-dome brightness through
 * sunrise -> noon -> sunset -> night. The placed SkyAtmosphere + real-time-capture SkyLight follow the
 * sun automatically; this actor additionally spawns its own moon directional light and an additive star
 * dome it drives via a dynamic material instance.
 *
 * Cosmetic only. Spawned at runtime by ATownGameMode (it is not saved into L_Town, so the hand-edited
 * town scene is untouched), and it flips the sun to Movable in code (no level / build_town.py change).
 * The dungeon uses a separate game mode, so its lighting is unaffected.
 */
UCLASS()
class DUNGEONCRAWLER_API ADayNightCycle : public AActor
{
	GENERATED_BODY()

public:
	ADayNightCycle();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// ---- Cycle timing ----
	/** Real seconds for one full day->night->day. Default ~2 min for an obvious, demoable cycle. */
	UPROPERTY(EditAnywhere, Category = "DayNight", meta = (ClampMin = "5"))
	float DayLengthSeconds = 120.f;

	/** Time of day to start at (0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset). */
	UPROPERTY(EditAnywhere, Category = "DayNight", meta = (ClampMin = "0", ClampMax = "1"))
	float StartTimeOfDay = 0.30f;

	/** Compass heading (yaw, degrees) the sun and moon travel along. */
	UPROPERTY(EditAnywhere, Category = "DayNight")
	float SunYaw = -35.f;

	// ---- Sun ----
	UPROPERTY(EditAnywhere, Category = "DayNight|Sun")
	float SunPeakIntensity = 6.f;
	UPROPERTY(EditAnywhere, Category = "DayNight|Sun")
	FLinearColor SunNoonColor = FLinearColor(1.f, 0.96f, 0.88f);
	UPROPERTY(EditAnywhere, Category = "DayNight|Sun")
	FLinearColor SunHorizonColor = FLinearColor(1.f, 0.45f, 0.20f);

	// ---- Moon (spawned by this actor) ----
	UPROPERTY(EditAnywhere, Category = "DayNight|Moon")
	float MoonPeakIntensity = 0.6f;
	UPROPERTY(EditAnywhere, Category = "DayNight|Moon")
	FLinearColor MoonColor = FLinearColor(0.5f, 0.6f, 1.f);

	// ---- Ambient sky-light floor (so night is never pitch black) ----
	UPROPERTY(EditAnywhere, Category = "DayNight|Ambient")
	float MinSkyLightIntensity = 0.6f;
	UPROPERTY(EditAnywhere, Category = "DayNight|Ambient")
	float MaxSkyLightIntensity = 1.f;

	// ---- Fog ----
	UPROPERTY(EditAnywhere, Category = "DayNight|Fog")
	FLinearColor FogDayColor = FLinearColor(0.6f, 0.7f, 0.9f);
	UPROPERTY(EditAnywhere, Category = "DayNight|Fog")
	FLinearColor FogNightColor = FLinearColor(0.02f, 0.03f, 0.08f);

	// ---- Stars ----
	UPROPERTY(EditAnywhere, Category = "DayNight|Stars")
	float StarMaxBrightness = 1.f;
	/** Night-ness (0..1) at which stars START to appear. Kept above 0 so stars only show in deep night and
	    are fully gone before dawn/dusk brightens the sky — avoids low-opacity star shimmer over a bright sky. */
	UPROPERTY(EditAnywhere, Category = "DayNight|Stars", meta = (ClampMin = "0", ClampMax = "1"))
	float StarNightThreshold = 0.18f;
	/** Inverted dome mesh for the stars (engine sphere by default; the star material is two-sided/additive). */
	UPROPERTY(EditAnywhere, Category = "DayNight|Stars")
	TSoftObjectPtr<UStaticMesh> StarDomeMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	/** Additive, unlit, two-sided star material exposing a "StarBrightness" scalar (authored by Tools/make_stars_material.py). */
	UPROPERTY(EditAnywhere, Category = "DayNight|Stars")
	TSoftObjectPtr<UMaterialInterface> StarMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/World/M_Stars.M_Stars")));
	/** Uniform scale of the star dome (large so it reads as a distant sky). */
	UPROPERTY(EditAnywhere, Category = "DayNight|Stars")
	float StarDomeScale = 800.f;

private:
	/** The star dome (root); the actor centers it on the town. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StarDome;

	UPROPERTY() TObjectPtr<UDirectionalLightComponent> SunComp;     // the placed town sun (found at BeginPlay)
	UPROPERTY() TObjectPtr<ADirectionalLight> MoonLight;            // spawned by this actor
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLightComp;        // found at BeginPlay
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> FogComp; // found at BeginPlay
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> StarMID;

	float TimeOfDay = 0.f; // 0..1

	/** Applies all lighting for a given normalized time of day. */
	void Apply(float Time01);
};
