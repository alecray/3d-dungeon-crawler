#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillTypes.h"
#include "SkillTreeComponent.generated.h"

class UStatsComponent;

/** Broadcast whenever allocations change so the skill UI can refresh. */
DECLARE_MULTICAST_DELEGATE(FOnSkillsChanged);

/**
 * Tracks which skill-tree nodes the player has allocated and applies their passive bonuses to the
 * owner's UStatsComponent. Allocation spends a skill point and requires the node's direct prereqs to
 * already be allocated (no per-tier point gating). Persisted via the player profile.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUNGEONCRAWLER_API USkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ranks currently invested in a node (0 if none). */
	int32 GetRank(FName NodeId) const;

	/** True if every prerequisite of the node has at least one rank. */
	bool ArePrereqsMet(const FSkillNode& Node) const;

	/** True if the node can be allocated now (exists, rank < max, prereqs met, a skill point free). */
	bool CanAllocate(FName NodeId) const;

	/** Spends a skill point to add a rank to the node. Returns true on success. */
	bool Allocate(FName NodeId);

	/** Flattened allocation list (one entry per rank) for saving. */
	TArray<FName> GetAllocationList() const;

	/** Rebuilds ranks from a saved allocation list and re-applies bonuses (does not spend points). */
	void LoadFrom(const TArray<FName>& AllocatedNodes);

	/** Summed combat modifiers from all allocated nodes (read by the player's combat code). */
	const FSkillModifiers& GetModifiers() const { return Modifiers; }

	FOnSkillsChanged OnSkillsChanged;

private:
	/** Sums allocated nodes' per-rank passive bonuses (to stats) and combat modifiers (cached here). */
	void RecomputeBonuses();
	UStatsComponent* GetStats() const;

	TMap<FName, int32> Ranks;
	FSkillModifiers Modifiers;
};
