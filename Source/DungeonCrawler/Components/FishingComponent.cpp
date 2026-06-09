#include "FishingComponent.h"

#include "FirstPersonCharacter.h"
#include "InventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"

UFishingComponent::UFishingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFishingComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerChar = Cast<AFirstPersonCharacter>(GetOwner());

	// Force-load the rod + anims up front so the .Get() at each use site is valid. An unset/unauthored
	// clip resolves to null, which the play call no-ops.
	FishingRodSkeletalAsset.LoadSynchronous();
	FishingCastAnim.LoadSynchronous();
	FishingIdleAnim.LoadSynchronous();
	FishingTensionAnim.LoadSynchronous();
	FishingReelAnim.LoadSynchronous();
}

void UFishingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Fade the status line when NOT actively fishing (transient messages like "Need a Fishing Rod").
	// While fishing it persists until the next state change / EndFishing.
	if (!bFishing && FishingStatusLeft > 0.f)
	{
		FishingStatusLeft -= DeltaTime;
		if (FishingStatusLeft <= 0.f) { FishingStatus.Reset(); }
	}
}

bool UFishingComponent::HasFishingRod() const
{
	UInventoryComponent* Inv = OwnerChar ? OwnerChar->GetInventoryComponent() : nullptr;
	return Inv && Inv->HasItem(FName(TEXT("FishingRod")));
}

void UFishingComponent::BeginFishing()
{
	bFishing = true;
	if (USkeletalMeshComponent* Held = OwnerChar ? OwnerChar->GetHeldWeaponMesh() : nullptr)
	{
		Held->SetSkeletalMesh(FishingRodSkeletalAsset.Get());
		Held->SetVisibility(FishingRodSkeletalAsset.Get() != nullptr); // null until the rod mesh is imported
	}
}

void UFishingComponent::PlayFishingPose(EFishingPose Pose)
{
	USkeletalMeshComponent* Held = OwnerChar ? OwnerChar->GetHeldWeaponMesh() : nullptr;
	if (!Held) { return; }

	UAnimSequence* Anim = nullptr;
	bool bLoop = false;
	switch (Pose)
	{
	case EFishingPose::Cast:    Anim = FishingCastAnim.Get();    bLoop = false; break;
	case EFishingPose::Idle:    Anim = FishingIdleAnim.Get();    bLoop = true;  break;
	case EFishingPose::Tension: Anim = FishingTensionAnim.Get(); bLoop = true;  break;
	case EFishingPose::Reel:    Anim = FishingReelAnim.Get();    bLoop = false; break;
	}
	if (Anim) { Held->PlayAnimation(Anim, bLoop); } // no-op until the anim exists
}

void UFishingComponent::EndFishing()
{
	bFishing = false;
	// Don't clear the status here — let it fade on its own (TickComponent fades it once !bFishing) so the
	// final "Caught a Bass!" / "It got away..." line stays readable for a moment after the rod is stowed.
	if (OwnerChar) { OwnerChar->EquipActiveHotbarItem(); } // restore the held weapon for the active hotbar slot
}

void UFishingComponent::SetFishingStatus(const FString& Status)
{
	FishingStatus = Status;
	FishingStatusLeft = FishingStatusDuration;
}
