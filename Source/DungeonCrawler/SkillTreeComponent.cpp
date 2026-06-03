#include "SkillTreeComponent.h"
#include "StatsComponent.h"

int32 USkillTreeComponent::GetRank(FName NodeId) const
{
	const int32* Found = Ranks.Find(NodeId);
	return Found ? *Found : 0;
}

bool USkillTreeComponent::ArePrereqsMet(const FSkillNode& Node) const
{
	for (const FName& Prereq : Node.Prereqs)
	{
		if (GetRank(Prereq) <= 0)
		{
			return false;
		}
	}
	return true;
}

bool USkillTreeComponent::CanAllocate(FName NodeId) const
{
	if (!SkillDatabase::Contains(NodeId))
	{
		return false;
	}
	const FSkillNode& Node = SkillDatabase::Get(NodeId);
	if (GetRank(NodeId) >= Node.MaxRank)
	{
		return false; // maxed
	}
	if (!ArePrereqsMet(Node))
	{
		return false;
	}
	const UStatsComponent* Stats = GetStats();
	return Stats && Stats->GetSkillPoints() > 0;
}

bool USkillTreeComponent::Allocate(FName NodeId)
{
	if (!CanAllocate(NodeId))
	{
		return false;
	}
	UStatsComponent* Stats = GetStats();
	if (!Stats || !Stats->SpendSkillPoint())
	{
		return false;
	}
	Ranks.FindOrAdd(NodeId) += 1;
	RecomputeBonuses(); // also fires Stats->NotifyChanged() (resizes resources, autosaves)
	OnSkillsChanged.Broadcast();
	return true;
}

TArray<FName> USkillTreeComponent::GetAllocationList() const
{
	TArray<FName> List;
	for (const TPair<FName, int32>& Pair : Ranks)
	{
		for (int32 r = 0; r < Pair.Value; ++r)
		{
			List.Add(Pair.Key);
		}
	}
	return List;
}

void USkillTreeComponent::LoadFrom(const TArray<FName>& AllocatedNodes)
{
	Ranks.Reset();
	for (const FName& NodeId : AllocatedNodes)
	{
		if (SkillDatabase::Contains(NodeId))
		{
			Ranks.FindOrAdd(NodeId) += 1;
		}
	}
	RecomputeBonuses();
	OnSkillsChanged.Broadcast();
}

bool USkillTreeComponent::HasAbility(EActiveAbility Ability) const
{
	if (Ability == EActiveAbility::None)
	{
		return false;
	}
	for (const TPair<FName, int32>& Pair : Ranks)
	{
		if (Pair.Value > 0 && SkillDatabase::Contains(Pair.Key)
			&& SkillDatabase::Get(Pair.Key).GrantsAbility == Ability)
		{
			return true;
		}
	}
	return false;
}

void USkillTreeComponent::RecomputeBonuses()
{
	UStatsComponent* Stats = GetStats();
	if (!Stats)
	{
		return;
	}

	FSkillBonuses Total;
	FSkillModifiers Mods;
	for (const TPair<FName, int32>& Pair : Ranks)
	{
		if (Pair.Value <= 0 || !SkillDatabase::Contains(Pair.Key))
		{
			continue;
		}
		const FSkillNode& Node = SkillDatabase::Get(Pair.Key);
		const FSkillBonuses& B = Node.PerRank;
		const FSkillModifiers& M = Node.ModsPerRank;
		const float Rank = static_cast<float>(Pair.Value);
		Total.MaxHealth  += B.MaxHealth  * Rank;
		Total.MaxMana    += B.MaxMana    * Rank;
		Total.MaxStamina += B.MaxStamina * Rank;
		Total.MeleeMult  += B.MeleeMult  * Rank;
		Total.RangedMult += B.RangedMult * Rank;
		Total.SpellMult  += B.SpellMult  * Rank;

		Mods.ExtraProjectiles += M.ExtraProjectiles * Pair.Value;
		Mods.AttackSpeedPct   += M.AttackSpeedPct   * Rank;
		Mods.StaminaCostPct   += M.StaminaCostPct   * Rank;
		Mods.ManaCostPct      += M.ManaCostPct      * Rank;
		Mods.LifestealPct     += M.LifestealPct     * Rank;
	}
	Modifiers = Mods;

	Stats->BonusMaxHealth  = Total.MaxHealth;
	Stats->BonusMaxMana    = Total.MaxMana;
	Stats->BonusMaxStamina = Total.MaxStamina;
	Stats->BonusMeleeMult  = Total.MeleeMult;
	Stats->BonusRangedMult = Total.RangedMult;
	Stats->BonusSpellMult  = Total.SpellMult;
	Stats->NotifyChanged();
}

UStatsComponent* USkillTreeComponent::GetStats() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UStatsComponent>() : nullptr;
}
