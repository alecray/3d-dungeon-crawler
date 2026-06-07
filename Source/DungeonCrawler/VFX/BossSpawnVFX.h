#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossSpawnVFX.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A dramatic, fully code-driven boss spawn-in effect (no art / Niagara — same lightweight style as
 * ADeathPoof / AImpactBurst). Spawned at the boss's feet when its intro begins and lives for the intro
 * duration: a ground light flares up, a tall energy pillar shoots up and fades, a column of glowing
 * shards streams upward in a swirl, and a debris ring bursts out at the base. Self-destructs at the end.
 *
 * NOTE: this is the "code VFX" spawn-in (option A). The richer dissolve/materialize SHADER version
 * (option B) needs a material asset and is tracked in TODO.md.
 */
UCLASS()
class DUNGEONCRAWLER_API ABossSpawnVFX : public AActor
{
	GENERATED_BODY()

public:
	ABossSpawnVFX();

	/** Sets the duration (match the boss intro) + energy color, and self-destructs after Duration. */
	void Configure(float Duration, const FLinearColor& Color);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Spawn") int32 ShardCount = 22;
	UPROPERTY(EditAnywhere, Category = "Spawn") float RiseSpeed = 540.f;   // shard upward speed (cm/s)
	UPROPERTY(EditAnywhere, Category = "Spawn") float SwirlRadius = 70.f;  // shard orbit radius at the base
	UPROPERTY(EditAnywhere, Category = "Spawn") float ColumnHeight = 360.f;// height shards travel before recycling
	UPROPERTY(EditAnywhere, Category = "Spawn") int32 DebrisCount = 10;
	UPROPERTY(EditAnywhere, Category = "Spawn") float GroundFlash = 9000.f;
	UPROPERTY(EditAnywhere, Category = "Spawn") float GroundFlashRadius = 700.f;

private:
	UPROPERTY() TObjectPtr<UPointLightComponent> GroundLight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Pillar;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Shards;  // rising swirl
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Debris;  // base burst ring

	// Per-shard phase data (parallel to Shards).
	TArray<float> ShardAngle;   // current orbit angle
	TArray<float> ShardSpin;    // angular speed
	TArray<float> ShardHeight;  // current height up the column
	TArray<float> ShardSpeed;   // per-shard rise speed

	TArray<FVector> DebrisVel;

	FLinearColor Tint = FLinearColor(1.0f, 0.5f, 0.15f);
	float Life = 1.8f;
	float Elapsed = 0.f;

	void TintMesh(UStaticMeshComponent* Comp) const;
};
