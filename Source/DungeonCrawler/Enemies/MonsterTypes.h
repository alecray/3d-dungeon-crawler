#pragma once

#include "CoreMinimal.h"
#include "MonsterTypes.generated.h"

class USkeletalMesh;
class UAnimSequence;
class AMonsterCharacter;

/**
 * Data definition for a kind of monster. Graybox by default (stats + body scale); if SkeletalMeshPath
 * resolves, the monster uses that mesh + a looping run animation instead of the cube body.
 */
USTRUCT()
struct FMonsterDef
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() float MaxHealth = 45.f;
	UPROPERTY() float MoveSpeed = 320.f;
	UPROPERTY() float AttackDamage = 12.f;
	UPROPERTY() float AttackRange = 170.f;
	UPROPERTY() float BodyScale = 1.f;   // graybox body scale
	UPROPERTY() float MeshScale = 1.f;   // skeletal-mesh visual scale (capsule auto-fits to it)
	UPROPERTY() float MeshYawOffset = -90.f; // mesh relative yaw; -90 matches the default UE import orientation
	UPROPERTY() int32 XPReward = 25;
	UPROPERTY() float CapsuleRadius = 40.f;
	UPROPERTY() float CapsuleHalfHeight = 88.f;

	/** Optional skeletal mesh + animation soft references (run loops, idle loops, attack plays once).
	    Soft refs (not raw path strings) so a moved/renamed asset is caught by reference fixup and
	    cook-time validation instead of silently failing to resolve at runtime. */
	UPROPERTY() TSoftObjectPtr<USkeletalMesh> SkeletalMeshPath;
	UPROPERTY() TSoftObjectPtr<UAnimSequence> RunAnimPath;
	UPROPERTY() TSoftObjectPtr<UAnimSequence> IdleAnimPath;
	UPROPERTY() TSoftObjectPtr<UAnimSequence> AttackAnimPath;
	UPROPERTY() TSoftObjectPtr<UAnimSequence> DeathAnimPath;
	/** Primary hit-react. If FlinchAnimAltPath is also set, the two are chosen at random on each hit. */
	UPROPERTY() TSoftObjectPtr<UAnimSequence> FlinchAnimPath;
	UPROPERTY() TSoftObjectPtr<UAnimSequence> FlinchAnimAltPath;

	/** Spawn weight for random selection. */
	UPROPERTY() float Weight = 1.f;

	/**
	 * The UObject class to spawn for this monster type. Defaults to AMonsterCharacter when null;
	 * set to a subclass (e.g. AFrogMonster) when the type needs custom movement or behaviour.
	 */
	UPROPERTY() TSubclassOf<AMonsterCharacter> MonsterClass;
};

/** Code-defined monster registry (no DataTable asset). */
namespace MonsterDatabase
{
	DUNGEONCRAWLER_API const FMonsterDef& Get(FName Id);
	DUNGEONCRAWLER_API bool Contains(FName Id);
	DUNGEONCRAWLER_API const TArray<FMonsterDef>& All();

	/** Weighted random monster type id. */
	DUNGEONCRAWLER_API FName RollRandomType(struct FRandomStream& Rng);
}
