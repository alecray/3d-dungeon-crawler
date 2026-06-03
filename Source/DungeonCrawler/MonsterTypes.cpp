#include "MonsterTypes.h"
#include "Math/RandomStream.h"

namespace MonsterDatabase
{
	static TArray<FMonsterDef> BuildTypes()
	{
		TArray<FMonsterDef> Types;

		// Crab — the default enemy. Skeletal mesh + run anim when imported; graybox otherwise.
		{
			FMonsterDef Crab;
			Crab.Id = TEXT("Crab");
			Crab.MaxHealth = 35.f;
			Crab.MoveSpeed = 360.f;
			Crab.AttackDamage = 8.f;
			Crab.AttackRange = 300.f; // attack from further out so the lunge in the anim closes the gap
			Crab.BodyScale = 0.9f;
			Crab.MeshScale = 0.5f; // halve the imported crab
			Crab.XPReward = 20;
			Crab.CapsuleRadius = 45.f;
			Crab.CapsuleHalfHeight = 50.f; // low and wide
			Crab.SkeletalMeshPath = TEXT("/Game/Enemies/SK_Crab.SK_Crab");
			Crab.RunAnimPath = TEXT("/Game/Enemies/A_Crab_Run.A_Crab_Run");
			Crab.IdleAnimPath = TEXT("/Game/Enemies/A_Crab_Idle.A_Crab_Idle");
			Crab.AttackAnimPath = TEXT("/Game/Enemies/A_Crab_Attack.A_Crab_Attack");
			Crab.Weight = 100.f;
			Types.Add(Crab);
		}

		// NOTE: only the Crab spawns for now (it replaced the graybox humanoid). Brute/Skitterer
		// definitions can be re-added with non-zero weights later for variety.

		return Types;
	}

	static const TArray<FMonsterDef>& Types()
	{
		static const TArray<FMonsterDef> Cached = BuildTypes();
		return Cached;
	}

	static const TMap<FName, int32>& IndexById()
	{
		static const TMap<FName, int32> Map = []()
		{
			TMap<FName, int32> M;
			const TArray<FMonsterDef>& List = Types();
			for (int32 i = 0; i < List.Num(); ++i)
			{
				M.Add(List[i].Id, i);
			}
			return M;
		}();
		return Map;
	}

	const FMonsterDef& Get(FName Id)
	{
		if (const int32* Index = IndexById().Find(Id))
		{
			return Types()[*Index];
		}
		static const FMonsterDef Invalid;
		return Invalid;
	}

	bool Contains(FName Id)
	{
		return IndexById().Contains(Id);
	}

	const TArray<FMonsterDef>& All()
	{
		return Types();
	}

	FName RollRandomType(FRandomStream& Rng)
	{
		const TArray<FMonsterDef>& List = Types();
		float Total = 0.f;
		for (const FMonsterDef& D : List) { Total += FMath::Max(0.f, D.Weight); }
		if (Total <= 0.f) { return List.Num() > 0 ? List[0].Id : NAME_None; }

		float Pick = Rng.FRandRange(0.f, Total);
		for (const FMonsterDef& D : List)
		{
			Pick -= FMath::Max(0.f, D.Weight);
			if (Pick <= 0.f) { return D.Id; }
		}
		return List[0].Id;
	}
}
