#include "SkillTypes.h"
#include <initializer_list>

namespace SkillDatabase
{
	static TArray<FSkillNode> BuildNodes()
	{
		auto Make = [](const TCHAR* Id, const TCHAR* Name, ESkillBranch Branch, int32 Tier, int32 MaxRank,
			const TCHAR* Desc, std::initializer_list<const TCHAR*> Prereqs, const FSkillBonuses& PerRank)
		{
			FSkillNode N;
			N.Id = FName(Id);
			N.DisplayName = Name;
			N.Description = Desc;
			N.Branch = Branch;
			N.Tier = Tier;
			N.Column = 0;
			N.MaxRank = MaxRank;
			N.PerRank = PerRank;
			for (const TCHAR* P : Prereqs) { N.Prereqs.Add(FName(P)); }
			return N;
		};

		auto Melee   = [](float V) { FSkillBonuses B; B.MeleeMult = V;  return B; };
		auto Ranged  = [](float V) { FSkillBonuses B; B.RangedMult = V; return B; };
		auto Spell   = [](float V) { FSkillBonuses B; B.SpellMult = V;  return B; };
		auto Health  = [](float V) { FSkillBonuses B; B.MaxHealth = V;  return B; };
		auto Mana    = [](float V) { FSkillBonuses B; B.MaxMana = V;    return B; };
		auto Stamina = [](float V) { FSkillBonuses B; B.MaxStamina = V; return B; };

		TArray<FSkillNode> Nodes;

		// ---- Melee branch ----
		Nodes.Add(Make(TEXT("melee_power"), TEXT("Power Strikes"), ESkillBranch::Melee, 0, 3,
			TEXT("+10% melee damage per rank."), {}, Melee(0.10f)));
		Nodes.Add(Make(TEXT("melee_vigor"), TEXT("Battle Vigor"), ESkillBranch::Melee, 1, 2,
			TEXT("+40 max health per rank."), { TEXT("melee_power") }, Health(40.f)));
		Nodes.Add(Make(TEXT("melee_brutality"), TEXT("Brutality"), ESkillBranch::Melee, 2, 1,
			TEXT("+25% melee damage."), { TEXT("melee_vigor") }, Melee(0.25f)));

		// ---- Ranged branch ----
		Nodes.Add(Make(TEXT("ranged_focus"), TEXT("Steady Aim"), ESkillBranch::Ranged, 0, 3,
			TEXT("+10% ranged damage per rank."), {}, Ranged(0.10f)));
		Nodes.Add(Make(TEXT("ranged_endurance"), TEXT("Endurance"), ESkillBranch::Ranged, 1, 2,
			TEXT("+30 max stamina per rank."), { TEXT("ranged_focus") }, Stamina(30.f)));
		Nodes.Add(Make(TEXT("ranged_deadeye"), TEXT("Deadeye"), ESkillBranch::Ranged, 2, 1,
			TEXT("+25% ranged damage."), { TEXT("ranged_endurance") }, Ranged(0.25f)));

		// ---- Mage branch ----
		Nodes.Add(Make(TEXT("mage_attune"), TEXT("Attunement"), ESkillBranch::Mage, 0, 3,
			TEXT("+10% spell damage per rank."), {}, Spell(0.10f)));
		Nodes.Add(Make(TEXT("mage_reservoir"), TEXT("Mana Reservoir"), ESkillBranch::Mage, 1, 2,
			TEXT("+40 max mana per rank."), { TEXT("mage_attune") }, Mana(40.f)));
		Nodes.Add(Make(TEXT("mage_archmage"), TEXT("Archmage"), ESkillBranch::Mage, 2, 1,
			TEXT("+25% spell damage."), { TEXT("mage_reservoir") }, Spell(0.25f)));

		return Nodes;
	}

	static const TArray<FSkillNode>& Nodes()
	{
		static const TArray<FSkillNode> Cached = BuildNodes();
		return Cached;
	}

	static const TMap<FName, int32>& IndexById()
	{
		static const TMap<FName, int32> Map = []()
		{
			TMap<FName, int32> M;
			const TArray<FSkillNode>& List = Nodes();
			for (int32 i = 0; i < List.Num(); ++i) { M.Add(List[i].Id, i); }
			return M;
		}();
		return Map;
	}

	const FSkillNode& Get(FName Id)
	{
		if (const int32* Index = IndexById().Find(Id))
		{
			return Nodes()[*Index];
		}
		static const FSkillNode Invalid;
		return Invalid;
	}

	bool Contains(FName Id) { return IndexById().Contains(Id); }

	const TArray<FSkillNode>& All() { return Nodes(); }

	const TCHAR* BranchName(ESkillBranch Branch)
	{
		switch (Branch)
		{
		case ESkillBranch::Melee:  return TEXT("Melee");
		case ESkillBranch::Ranged: return TEXT("Ranged");
		case ESkillBranch::Mage:   return TEXT("Mage");
		default:                   return TEXT("");
		}
	}
}
