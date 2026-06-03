#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;

/**
 * A travel portal (graybox arch). Interacting with it (E) saves the profile and opens the target
 * level. Used both for the town's "enter dungeon" portal and the dungeon's "return to town" portals.
 * Can start inactive (hidden, non-interactable) and be switched on later — e.g. after the boss dies.
 */
UCLASS()
class DUNGEONCRAWLER_API APortal : public AActor
{
	GENERATED_BODY()

public:
	APortal();

	/** Short level name to open on use (e.g. "L_Town" or "L_DungeonTest"). */
	UPROPERTY(EditAnywhere, Category = "Portal")
	FName TargetMapName;

	/** A portal can be placed but dormant; activate it later (the after-boss return portal). */
	UPROPERTY(EditAnywhere, Category = "Portal")
	bool bActive = true;

	FName GetTargetMapName() const { return TargetMapName; }
	bool IsActive() const { return bActive; }
	void SetActive(bool bInActive);
	void SetTargetMapName(FName InMap) { TargetMapName = InMap; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<USceneComponent> Root;

	/** Blocks the Visibility channel so the player's interact line-trace can target it. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Frame;

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UPointLightComponent> Glow;
};
