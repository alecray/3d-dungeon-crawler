#include "StatsComponent.h"

UStatsComponent::UStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

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

void UStatsComponent::LoadFrom(const FCoreAttributes& InAttributes, int32 InLevel, int32 InXP, int32 InSkillPoints, int32 InAttributePoints)
{
	Attributes = InAttributes;
	Level = FMath::Max(1, InLevel);
	XP = FMath::Max(0, InXP);
	SkillPoints = FMath::Max(0, InSkillPoints);
	AttributePoints = FMath::Max(0, InAttributePoints);
	NotifyChanged();
}
