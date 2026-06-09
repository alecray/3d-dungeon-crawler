#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycle.generated.h"

class ADirectionalLight;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class USceneComponent;

/**
 * Town-only day/night cycle. Rotates the level's existing sun over a configurable day length and lerps
 * sun/moon color + intensity, the sky-light floor, and fog color through sunrise -> noon -> sunset ->
 * night. The placed SkyAtmosphere + real-time-capture SkyLight follow the sun automatically; night look
 * is carried by SkyAtmosphere + the moon directional light spawned by this actor.
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

	/** Normalized time of day: 0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset. */
	float GetTimeOfDay() const { return TimeOfDay; }

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
	float SunPeakIntensity = 4.5f; // toned down from 6 — daytime was a bit too bright
	UPROPERTY(EditAnywhere, Category = "DayNight|Sun")
	FLinearColor SunNoonColor = FLinearColor(1.f, 0.89f, 0.72f); // warmer, less stark white
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

private:
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> SunComp;     // the placed town sun (found at BeginPlay)
	UPROPERTY() TObjectPtr<ADirectionalLight> MoonLight;            // spawned by this actor
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLightComp;        // found at BeginPlay
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> FogComp; // found at BeginPlay

	float TimeOfDay = 0.f; // 0..1

	/** Applies all lighting for a given normalized time of day. */
	void Apply(float Time01);
};
