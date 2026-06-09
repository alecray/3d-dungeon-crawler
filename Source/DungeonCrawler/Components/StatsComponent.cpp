#include "StatsComponent.h"

UStatsComponent::UStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Grants XP and rolls over as many levels as the total allows (a big kill/quest reward can level up
// more than once in a single call), carrying the remainder into the next level's bar.
void UStatsComponent::AddXP(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	XP += Amount;
	while (XP >= XPToNextLevel(Level))
	{
		XP -= XPToNextLevel(Level);
		LevelUp();
	}
	NotifyChanged();
}

// Per-level rewards: 1 skill-tree point and 3 attribute points to spend on STR/INT/DEX/VIT.
void UStatsComponent::LevelUp()
{
	++Level;
	SkillPoints += 1;
	AttributePoints += 3;
}

bool UStatsComponent::SpendSkillPoint()
{
	if (SkillPoints <= 0)
	{
		return false;
	}
	--SkillPoints;
	return true;
}

// Spends one unspent attribute point into the referenced attribute (no-op when none are banked).
// The Spend* wrappers below pin this to a specific stat so the UI can bind simple click handlers.
void UStatsComponent::AddAttributePoint(int32& Attribute)
{
	if (AttributePoints <= 0)
	{
		return;
	}
	--AttributePoints;
	++Attribute;
	NotifyChanged();
}

void UStatsComponent::SpendOnStrength()     { AddAttributePoint(Attributes.Strength); }
void UStatsComponent::SpendOnIntelligence() { AddAttributePoint(Attributes.Intelligence); }
void UStatsComponent::SpendOnDexterity()    { AddAttributePoint(Attributes.Dexterity); }
void UStatsComponent::SpendOnVitality()     { AddAttributePoint(Attributes.Vitality); }

// Restores progression from a saved profile, clamping each field to a legal minimum (Level >= 1, the
// rest >= 0) so a corrupt/edited save can't produce a broken character.
void UStatsComponent::LoadFrom(const FCoreAttributes& InAttributes, int32 InLevel, int32 InXP, int32 InSkillPoints, int32 InAttributePoints)
{
	Attributes = InAttributes;
	Level = FMath::Max(1, InLevel);
	XP = FMath::Max(0, InXP);
	SkillPoints = FMath::Max(0, InSkillPoints);
	AttributePoints = FMath::Max(0, InAttributePoints);
	NotifyChanged();
}
