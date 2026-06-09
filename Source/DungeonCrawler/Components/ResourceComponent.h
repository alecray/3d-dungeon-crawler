#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceComponent.generated.h"

/**
 * A regenerating pool (mana, stamina, …): current/max value with passive regen and Spend/Restore.
 * Health/damage/death stays in UHealthComponent; this is for spendable resources.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Spends Amount if available; returns true if it could be paid in full. */
	bool Spend(float Amount);

	/** Subtracts Amount (clamped to 0); for continuous drains like sprinting. Always succeeds. */
	void Drain(float Amount);

	/** Adds Amount back, clamped to Max. */
	void Restore(float Amount);

	/** Sets a new max; keeps the same fill fraction unless bRefill, which tops it off. */
	void SetMax(float NewMax, bool bRefill = false);

	/** Sets the current value directly (clamped to [0, Max]); e.g. revive to a fraction of max. */
	void SetCurrent(float Value);

	float GetCurrent() const { return Current; }
	float GetMax() const { return MaxValue; }
	float GetPercent() const { return MaxValue > 0.f ? Current / MaxValue : 0.f; }
	bool Has(float Amount) const { return Current >= Amount; }

	/** Pause/resume passive regen (e.g. while sprinting drains stamina). */
	void SetRegenPaused(bool bPaused) { bRegenPaused = bPaused; }

	void SetRegenPerSecond(float V) { RegenPerSecond = FMath::Max(0.f, V); }
	void SetRegenDelay(float V)     { RegenDelay     = FMath::Max(0.f, V); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "1"))
	float MaxValue = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	float Current = 0.f;

	/** Points restored per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0"))
	float RegenPerSecond = 12.f;

	/** Seconds to wait after the last Spend() before regen resumes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0"))
	float RegenDelay = 1.0f;

private:
	bool bRegenPaused = false;
	float LastSpendTime = -1000.f;
};
