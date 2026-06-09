#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.generated.h"

/** The three skill-tree branches, aligned with the combat styles. */
UENUM(BlueprintType)
enum class ESkillBranch : uint8
{
	Melee,
	Ranged,
	Mage
};

/** Active abilities unlocked by capstone nodes (triggered with the ability key for the matching style). */
UENUM(BlueprintType)
enum class EActiveAbility : uint8
{
	None,
	Whirlwind, // melee: AoE sweep around the player
	Volley,    // ranged: a wide burst of bolts
	Nova       // mage: AoE burst around the player
};

/** Passive stat bonuses granted by a node, applied per allocated rank to UStatsComponent. */
USTRUCT(BlueprintType)
struct FSkillBonuses
{
	GENERATED_BODY()

	UPROPERTY() float MaxHealth = 0.f;
	UPROPERTY() float MaxMana = 0.f;
	UPROPERTY() float MaxStamina = 0.f;
	UPROPERTY() float MeleeMult = 0.f;  // additive to the melee damage multiplier
	UPROPERTY() float RangedMult = 0.f;
	UPROPERTY() float SpellMult = 0.f;
};

/** Combat modifiers granted by a node (per rank, additive); read by the player's combat code. */
USTRUCT(BlueprintType)
struct FSkillModifiers
{
	GENERATED_BODY()

	UPROPERTY() int32 ExtraProjectiles = 0; // extra bolts per shot (ranged/mage), fired in a spread
	UPROPERTY() float AttackSpeedPct = 0.f; // shortens attack cooldown: cooldown /= (1 + total)
	UPROPERTY() float StaminaCostPct = 0.f; // fraction off ranged stamina cost (capped)
	UPROPERTY() float ManaCostPct = 0.f;    // fraction off mage mana cost (capped)
	UPROPERTY() float LifestealPct = 0.f;   // melee heal as a fraction of damage dealt
};

/**
 * Static definition of one skill-tree node. Nodes live in three branches (Melee/Ranged/Mage); a node
 * can be allocated once its direct prerequisites are allocated (no per-tier point gating). For now
 * nodes grant passive stat bonuses; combat modifiers and active abilities layer on in later steps.
 */
USTRUCT(BlueprintType)
struct FSkillNode
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() FString DisplayName;
	UPROPERTY() FString Description;
	UPROPERTY() ESkillBranch Branch = ESkillBranch::Melee;

	UPROPERTY() int32 Tier = 0;   // row within the branch (0 = top), for UI layout
	UPROPERTY() int32 Column = 0; // horizontal offset within the branch, for UI layout
	UPROPERTY() int32 MaxRank = 1;

	UPROPERTY() TArray<FName> Prereqs; // node ids that must be allocated first
	UPROPERTY() FSkillBonuses PerRank; // passive bonus added for each allocated rank
	UPROPERTY() FSkillModifiers ModsPerRank; // combat modifiers added for each allocated rank
	UPROPERTY() EActiveAbility GrantsAbility = EActiveAbility::None; // active ability unlocked by this node
};

/**
 * Code-defined skill registry (mirrors ItemDatabase / MonsterDatabase): look up nodes by id and
 * enumerate the tree. Defined in C++ so the tree stays diff-friendly.
 */
namespace SkillDatabase
{
	DUNGEONCRAWLER_API const FSkillNode& Get(FName Id);
	DUNGEONCRAWLER_API bool Contains(FName Id);
	DUNGEONCRAWLER_API const TArray<FSkillNode>& All();

	/** Display label for a branch (e.g. "Melee"). */
	DUNGEONCRAWLER_API const TCHAR* BranchName(ESkillBranch Branch);
}
