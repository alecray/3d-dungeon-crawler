#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "MonsterSpawnPedestal.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class AMonsterCharacter;

/**
 * A small interactable pillar (E to open) used to spawn enemies in town for testing. The spawn menu lets
 * you pick any registered monster type or boss; the chosen creature appears in front of the pedestal as a
 * passive "test dummy" — it wanders idly around the spawn point and ignores the player until first struck,
 * at which point it turns fully hostile (normal AI).
 *
 * The pedestal itself is a skeletal mesh with a single "A_Spawn_Pedestal" animation that plays each time a
 * monster is spawned (falls back to a graybox cylinder until the FBX is imported).
 */
UCLASS()
class DUNGEONCRAWLER_API AMonsterSpawnPedestal : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawnPedestal();

	/** Spawns a registered monster type (MonsterDatabase id) as a passive roaming dummy. */
	AMonsterCharacter* SpawnMonsterType(FName TypeId);

	/** Spawns a boss class (e.g. ABossMonster) as a passive roaming dummy (tier 0). */
	AMonsterCharacter* SpawnBossClass(TSubclassOf<AMonsterCharacter> BossClass);

	/** Destroys every dummy this pedestal has spawned (the menu's "Clear" button). */
	void ClearSpawned();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Pedestal")
	TObjectPtr<UCapsuleComponent> Body; // blocks Visibility so the interact trace can target it

	/** The pedestal model + its single spawn animation. */
	UPROPERTY(VisibleAnywhere, Category = "Pedestal")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Graybox stand-in shown until the real pedestal mesh is imported. */
	UPROPERTY(VisibleAnywhere, Category = "Pedestal")
	TObjectPtr<UStaticMeshComponent> Graybox;

	/** Pedestal skeletal mesh (import the FBX here, or repoint this). */
	UPROPERTY(EditAnywhere, Category = "Pedestal")
	TSoftObjectPtr<USkeletalMesh> PedestalMeshPath = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Town/SpawnPedestal/SK_Spawn_Pedestal.SK_Spawn_Pedestal")));

	/** The single "A_Spawn_Pedestal" animation, played once per spawn. */
	UPROPERTY(EditAnywhere, Category = "Pedestal")
	TSoftObjectPtr<UAnimSequence> SpawnAnimPath = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Town/SpawnPedestal/A_Spawn_Pedestal.A_Spawn_Pedestal")));

	/** How far in front of the pedestal dummies appear (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestal", meta = (ClampMin = "50"))
	float SpawnForwardOffset = 280.f;

	/** Radius (cm) the spawned dummies wander within around their spawn point. */
	UPROPERTY(EditAnywhere, Category = "Pedestal", meta = (ClampMin = "0"))
	float RoamRadius = 450.f;

private:
	/** Common post-spawn setup: passive + roam home, track it, play the pedestal anim. */
	AMonsterCharacter* FinishSpawn(AMonsterCharacter* Monster, const FVector& Loc);
	FVector GetSpawnLocation() const;
	void PlaySpawnAnim();

	UPROPERTY() TObjectPtr<UAnimSequence> SpawnAnim; // resolved from SpawnAnimPath in BeginPlay
	UPROPERTY() TArray<TObjectPtr<AMonsterCharacter>> Spawned;
};
