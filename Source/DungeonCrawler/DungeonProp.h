#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonProp.generated.h"

class UStaticMeshComponent;

/** Kinds of graybox furniture the dungeon can scatter through its rooms. */
UENUM(BlueprintType)
enum class EPropType : uint8
{
	Chair		UMETA(DisplayName = "Chair"),
	Table		UMETA(DisplayName = "Table"),
	Cabinet		UMETA(DisplayName = "Cabinet"),
	Dresser		UMETA(DisplayName = "Dresser"),
	Bookshelf	UMETA(DisplayName = "Bookshelf"),

	MAX			UMETA(Hidden)
};

/**
 * A single piece of graybox furniture, assembled from engine cube primitives based on PropType.
 * Each piece is a self-contained "prefab" actor: drop a Blender mesh onto MeshOverride (or replace
 * the assembled cubes) later without touching the generator. The base of the prop sits at Z = 0.
 */
UCLASS()
class DUNGEONCRAWLER_API ADungeonProp : public AActor
{
	GENERATED_BODY()

public:
	ADungeonProp();

	/** Sets the type and (re)builds the graybox geometry. Call right after spawning. */
	void Configure(EPropType InType);

	EPropType GetPropType() const { return PropType; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Which furniture this prop represents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop")
	EPropType PropType = EPropType::Chair;

	/**
	 * Optional finished mesh. If set, the graybox cubes are skipped and this single mesh is used
	 * instead — the swap-in point for Blender assets.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop")
	TObjectPtr<UStaticMesh> MeshOverride;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	/** Clears existing parts and rebuilds from PropType (or MeshOverride). */
	void Rebuild();

	/**
	 * Adds one graybox box.
	 * @param Center   center of the box in cm, relative to the prop root.
	 * @param SizeCm   full size of the box in cm along X/Y/Z.
	 */
	UStaticMeshComponent* AddBox(const FVector& Center, const FVector& SizeCm);
};
