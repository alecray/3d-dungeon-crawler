#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatsComponent.generated.h"

class UStatsComponent;

/** Broadcast whenever attributes/level change so dependents (resource maxes, HUD) can refresh. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatsChanged, UStatsComponent*);

/** Core RPG attributes. Kept as a plain struct so it can be saved/loaded as one blob. */
USTRUCT(BlueprintType)
struct FCoreAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes") int32 Strength = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes") int32 Intelligence = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes") int32 Dexterity = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes") int32 Vitality = 5;
};

/**
 * Player progression: core attributes (STR/INT/DEX/VIT), level/XP/points, and the derived numbers
 * the rest of the game reads (max health/mana/stamina, damage multipliers). Other systems (skill
 * tree, gear) layer bonuses on top via the Bonus* setters.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API UStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatsComponent();

	// ---- Attributes ----
	const FCoreAttributes& GetAttributes() const { return Attributes; }
	void SetAttributes(const FCoreAttributes& In) { Attributes = In; NotifyChanged(); }
	void AddAttributePoint(int32& Attribute);

	int32 GetStrength() const { return Attributes.Strength; }
	int32 GetIntelligence() const { return Attributes.Intelligence; }
	int32 GetDexterity() const { return Attributes.Dexterity; }
	int32 GetVitality() const { return Attributes.Vitality; }

	// Spend an unspent attribute point into a stat (used by a future stats screen).
	void SpendOnStrength();
	void SpendOnIntelligence();
	void SpendOnDexterity();
	void SpendOnVitality();

	// ---- Level / XP ----
	void AddXP(int32 Amount);
	int32 GetLevel() const { return Level; }
	int32 GetXP() const { return XP; }
	int32 GetXPToNext() const { return XPToNextLevel(Level); }
	int32 GetSkillPoints() const { return SkillPoints; }
	int32 GetAttributePoints() const { return AttributePoints; }
	bool SpendSkillPoint();

	// ---- Derived (attributes + flat bonuses from skills + equipment) ----
	// Each derived stat = a flat base + a per-point attribute scaling + flat bonuses from two separate
	// sources (skill tree, equipped gear). The damage mults are 1.0 at the base and add +5% per relevant
	// attribute point, so they stay sensible multipliers (e.g. STR 9 -> 1.45x melee).
	float GetMaxHealth() const { return 80.f + Attributes.Vitality * 20.f + BonusMaxHealth + EquipBonusMaxHealth; }
	float GetMaxMana() const { return 40.f + Attributes.Intelligence * 15.f + BonusMaxMana + EquipBonusMaxMana; }
	float GetMaxStamina() const { return 80.f + Attributes.Dexterity * 8.f + Attributes.Vitality * 4.f + BonusMaxStamina + EquipBonusMaxStamina; }
	float GetMeleeDamageMult() const { return 1.f + Attributes.Strength * 0.05f + BonusMeleeMult + EquipBonusMeleeMult; }
	float GetRangedDamageMult() const { return 1.f + Attributes.Dexterity * 0.05f + BonusRangedMult + EquipBonusRangedMult; }
	float GetSpellDamageMult() const { return 1.f + Attributes.Intelligence * 0.05f + BonusSpellMult + EquipBonusSpellMult; }

	// Flat bonuses layered on by the skill tree (set then NotifyChanged()).
	float BonusMaxHealth = 0.f, BonusMaxMana = 0.f, BonusMaxStamina = 0.f;
	float BonusMeleeMult = 0.f, BonusRangedMult = 0.f, BonusSpellMult = 0.f;

	// Flat bonuses layered on by equipped gear (separate source so it doesn't clobber skill bonuses).
	float EquipBonusMaxHealth = 0.f, EquipBonusMaxMana = 0.f, EquipBonusMaxStamina = 0.f;
	float EquipBonusMeleeMult = 0.f, EquipBonusRangedMult = 0.f, EquipBonusSpellMult = 0.f;

	void NotifyChanged() { OnStatsChanged.Broadcast(this); }
	FOnStatsChanged OnStatsChanged;

	// Save/load round-trip.
	void LoadFrom(const FCoreAttributes& InAttributes, int32 InLevel, int32 InXP, int32 InSkillPoints, int32 InAttributePoints);

protected:
	UPROPERTY(EditAnywhere, Category = "Stats")
	FCoreAttributes Attributes;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	int32 XP = 0;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	int32 SkillPoints = 0;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	int32 AttributePoints = 0;

private:
	// Linear XP curve: 100 XP for level 1->2, then +75 per level after that (175, 250, ...).
	static int32 XPToNextLevel(int32 ForLevel) { return 100 + (ForLevel - 1) * 75; }
	void LevelUp();
};
