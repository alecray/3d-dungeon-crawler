#pragma once

#include "CoreMinimal.h"
#include "MonsterCharacter.h"
#include "BossMonster.generated.h"

class UStaticMeshComponent;

/**
 * Oversized boss built on top of AMonsterCharacter (it inherits the chase/attack/health/hit-react).
 * Adds a 3-phase fight: as its health crosses thresholds it advances phase, morphs by revealing an
 * extra cube section, and buffs its stats. On a randomized timer it also fires a random special
 * mechanic (ground slam / summon adds / enrage burst).
 */
UCLASS()
class DUNGEONCRAWLER_API ABossMonster : public AMonsterCharacter
{
	GENERATED_BODY()

public:
	ABossMonster();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Health fraction at/below which the boss enters phase 2 and phase 3. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase2HealthPct = 0.66f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase3HealthPct = 0.33f;

	// ---- Special mechanics ----
	UPROPERTY(EditAnywhere, Category = "Boss")
	float SpecialMinInterval = 4.f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float SpecialMaxInterval = 7.f;

	/** Ground-slam radius (cm) and damage. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	float SlamRadius = 450.f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float SlamDamage = 22.f;

	/** How many adds a summon spawns. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	int32 SummonCount = 2;

private:
	void AdvanceToPhase(int32 NewPhase);
	void ScheduleNextSpecial();
	void DoRandomSpecial();

	void SlamAttack();
	void SummonAdds();
	void EnrageBurst();

	/** Three cube "growths" revealed one-per-phase to show the boss morphing. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> MorphParts;

	int32 CurrentPhase = 0;
	float NextSpecialTime = 0.f;
	float EnrageEndTime = 0.f;
	float BaseMoveSpeed = 0.f;
};
