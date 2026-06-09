#include "CasinoCat.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "TimerManager.h"

// ---- constructor ----------------------------------------------------------------

ACasinoCat::ACasinoCat()
{
	PrimaryActorTick.bCanEverTick = false; // purely timer-driven; no per-frame work needed

	CatMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CatMesh"));
	SetRootComponent(CatMesh);

	// Cosmetic prop — no collision, but casts a shadow so it reads in the scene lighting.
	CatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CatMesh->SetCastShadow(true);

	// Single-node animation mode: we drive it ourselves via PlayAnimation; no AnimBP required.
	CatMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Default asset paths — derived from Content/NPCs/ discovery:
	//   Mesh:   /Game/NPCs/SK_Casino_Cat
	//   Idle_0: /Game/NPCs/A_Casino_Cat_Idle_0
	//   Idle_1: /Game/NPCs/A_Casino_Cat_Idle_1
	//   Idle_2: /Game/NPCs/A_Casino_Cat_Idle_2
	//   Idle_3: /Game/NPCs/A_Casino_Cat_Idle_3
	SkeletalMeshAsset = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(TEXT("/Game/NPCs/SK_Casino_Cat.SK_Casino_Cat")));

	Idle0 = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/NPCs/A_Casino_Cat_Idle_0.A_Casino_Cat_Idle_0")));
	Idle1 = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/NPCs/A_Casino_Cat_Idle_1.A_Casino_Cat_Idle_1")));
	Idle2 = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/NPCs/A_Casino_Cat_Idle_2.A_Casino_Cat_Idle_2")));
	Idle3 = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/NPCs/A_Casino_Cat_Idle_3.A_Casino_Cat_Idle_3")));
	Idle4 = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/NPCs/A_Casino_Cat_Idle_4.A_Casino_Cat_Idle_4")));
	DealAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/NPCs/A_Casino_Cat_Deal.A_Casino_Cat_Deal")));
}

// ---- BeginPlay ------------------------------------------------------------------

void ACasinoCat::BeginPlay()
{
	Super::BeginPlay();

	// Load the skeletal mesh and apply it to the component.
	if (!SkeletalMeshAsset.IsNull())
	{
		if (USkeletalMesh* Skel = SkeletalMeshAsset.LoadSynchronous())
		{
			CatMesh->SetSkeletalMesh(Skel);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ACasinoCat [%s]: skeletal mesh failed to load — check SkeletalMeshAsset path (%s)."),
				*GetName(), *SkeletalMeshAsset.ToString());
		}
	}

	// Load the three idle clips.  Missing clips are skipped gracefully in PlayWeightedIdle.
	auto TryLoad = [&](const TSoftObjectPtr<UAnimSequence>& Ref, const TCHAR* Label) -> UAnimSequence*
	{
		if (Ref.IsNull()) { return nullptr; }
		UAnimSequence* Anim = Ref.LoadSynchronous();
		if (!Anim)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ACasinoCat [%s]: %s anim failed to load (%s) — it will be skipped."),
				*GetName(), Label, *Ref.ToString());
		}
		return Anim;
	};

	LoadedIdle0 = TryLoad(Idle0, TEXT("Idle_0"));
	LoadedIdle1 = TryLoad(Idle1, TEXT("Idle_1"));
	LoadedIdle2 = TryLoad(Idle2, TEXT("Idle_2"));
	LoadedIdle3 = TryLoad(Idle3, TEXT("Idle_3"));
	LoadedIdle4 = TryLoad(Idle4, TEXT("Idle_4"));
	LoadedDeal = TryLoad(DealAnim, TEXT("Deal"));

	// Kick off the rolling idle loop immediately.
	PlayWeightedIdle();
}

void ACasinoCat::PlayDeal()
{
	// No deal clip imported yet -> leave the idle loop running.
	if (!LoadedDeal || !CatMesh)
	{
		return;
	}

	UWorld* World = GetWorld();
	// Interrupt the idle loop (cancel the pending re-roll), play the deal once, then resume idles when it ends.
	if (World)
	{
		World->GetTimerManager().ClearTimer(IdleTimerHandle);
	}
	CatMesh->PlayAnimation(LoadedDeal, /*bLooping*/ false);

	const float DealLength = FMath::Max(0.05f, LoadedDeal->GetPlayLength());
	if (World)
	{
		World->GetTimerManager().SetTimer(
			IdleTimerHandle, this, &ACasinoCat::PlayWeightedIdle, DealLength, /*bLooping*/ false);
	}
}

// ---- PlayWeightedIdle -----------------------------------------------------------

void ACasinoCat::PlayWeightedIdle()
{
	// Build a candidate list of (clip, weight) pairs — skip any that failed to load.
	struct FCandidate { UAnimSequence* Clip; float Weight; };
	TArray<FCandidate> Candidates;
	Candidates.Reserve(5);

	if (LoadedIdle0) { Candidates.Add({ LoadedIdle0, FMath::Max(0.f, Weight0) }); }
	if (LoadedIdle1) { Candidates.Add({ LoadedIdle1, FMath::Max(0.f, Weight1) }); }
	if (LoadedIdle2) { Candidates.Add({ LoadedIdle2, FMath::Max(0.f, Weight2) }); }
	if (LoadedIdle3) { Candidates.Add({ LoadedIdle3, FMath::Max(0.f, Weight3) }); }
	if (LoadedIdle4) { Candidates.Add({ LoadedIdle4, FMath::Max(0.f, Weight4) }); }

	if (Candidates.IsEmpty())
	{
		// Nothing to play — log once and give up (no timer re-arm so we don't spam).
		UE_LOG(LogTemp, Warning,
			TEXT("ACasinoCat [%s]: no idle animations loaded; cat will not animate."), *GetName());
		return;
	}

	// If only one clip is available, just loop it directly without a weighted roll.
	if (Candidates.Num() == 1)
	{
		CatMesh->PlayAnimation(Candidates[0].Clip, /*bLooping*/ true);
		// No timer needed — the single clip loops forever.
		return;
	}

	// Weighted random roll: pick a uniform float in [0, TotalWeight) and walk the table.
	float TotalWeight = 0.f;
	for (const FCandidate& C : Candidates) { TotalWeight += C.Weight; }

	UAnimSequence* Chosen = Candidates.Last().Clip; // fallback: last entry (should never be needed)
	if (TotalWeight > 0.f)
	{
		float Roll = FMath::FRandRange(0.f, TotalWeight);
		for (const FCandidate& C : Candidates)
		{
			Roll -= C.Weight;
			if (Roll <= 0.f)
			{
				Chosen = C.Clip;
				break;
			}
		}
	}

	// Play the chosen clip once (bLooping = false); the timer fires when it ends and re-rolls.
	CatMesh->PlayAnimation(Chosen, /*bLooping*/ false);

	const float ClipLength = FMath::Max(0.05f, Chosen->GetPlayLength());
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			IdleTimerHandle,
			this, &ACasinoCat::PlayWeightedIdle,
			ClipLength,
			/*bLooping*/ false);
	}
}
