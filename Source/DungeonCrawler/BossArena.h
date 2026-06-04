#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossArena.generated.h"

class UBoxComponent;
class ABossMonster;
class ABossDoor;
class APortal;
class ACameraActor;
class UHealthComponent;
class UBossHealthBarWidget;

/**
 * Drives a boss-room encounter. Spawned and configured by ADungeonGenerator over the boss room. When
 * the player first crosses its trigger it runs the encounter:
 *   1. spawns the boss on the far side of the room, opposite the player, and seals the entrance doors;
 *   2. on the FIRST-EVER encounter with that boss type (persisted), locks input and blends the camera
 *      to frame the boss with a screen shake during its intro animation, then blends back to the player;
 *   3. on replays it just plays the boss's spawn animation — no camera work or shake.
 * When the boss dies the doors re-open and the dormant return portal activates.
 */
UCLASS()
class DUNGEONCRAWLER_API ABossArena : public AActor
{
	GENERATED_BODY()

public:
	ABossArena();

	/** Configures the encounter. Called by the generator right after spawning. */
	void Configure(TSubclassOf<ABossMonster> InBossClass, const FVector& RoomCenterWorld, const FVector2D& RoomHalfXY,
		TSubclassOf<APortal> InPortalClass, FName InTownMap);

	/** Registers one entrance doorway to seal (world transform + full barrier size in cm). */
	void AddDoorSlot(const FTransform& WorldXf, const FVector& SizeCm);

protected:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UPROPERTY(VisibleAnywhere, Category = "BossArena")
	TObjectPtr<UBoxComponent> Trigger;

private:
	void StartEncounter(APawn* Player);
	void RunIntroCinematic(APawn* Player);
	void ReturnCameraToPlayer();
	void FinishCinematic();
	void OnBossDefeated(UHealthComponent* DeadComponent);

	struct FDoorSlot { FTransform Xf; FVector Size; };
	TArray<FDoorSlot> DoorSlots;

	UPROPERTY() TArray<TObjectPtr<ABossDoor>> Doors;
	UPROPERTY() TObjectPtr<ABossMonster> Boss;
	UPROPERTY() TObjectPtr<APortal> ReturnPortal;
	UPROPERTY() TObjectPtr<ACameraActor> IntroCamera;
	UPROPERTY() TObjectPtr<UBossHealthBarWidget> HealthBar;

	TSubclassOf<ABossMonster> BossClass;
	TSubclassOf<APortal> PortalClass;
	FName TownMapName;
	FVector RoomCenter = FVector::ZeroVector;
	FVector2D RoomHalf = FVector2D(600.f, 600.f);

	bool bStarted = false;

	FTimerHandle CameraReturnTimer;
	FTimerHandle FinishTimer;
};
