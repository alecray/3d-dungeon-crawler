#include "CombatComponent.h"

#include "FirstPersonCharacter.h"
#include "StatsComponent.h"
#include "ResourceComponent.h"
#include "HealthComponent.h"
#include "SkillTreeComponent.h"
#include "MonsterCharacter.h"
#include "Projectile.h"
#include "DeathPoof.h"
#include "DungeonGameInstance.h"
#include "HitCameraShake.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/App.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	ProjectileClass = AProjectile::StaticClass();
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerChar = Cast<AFirstPersonCharacter>(GetOwner());

	// Force-load the attack anims up front so the .Get() at each use site is valid (unauthored clips no-op).
	SwingAnim.LoadSynchronous();
	SwingAnimAlt.LoadSynchronous();
	DeflectAnim.LoadSynchronous();
	CrossbowShootAnim.LoadSynchronous();
	StaffCastAnim.LoadSynchronous();
}

void UCombatComponent::SetMeleeAnims(const TSoftObjectPtr<UAnimSequence>& Primary, const TSoftObjectPtr<UAnimSequence>& Alt)
{
	if (!Primary.IsNull())
	{
		SwingAnim = Primary;
		SwingAnim.LoadSynchronous();
	}
	SwingAnimAlt = Alt;
	if (!SwingAnimAlt.IsNull())
	{
		SwingAnimAlt.LoadSynchronous();
	}
	bNextSwingLeft = true;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Count hit-stop in REAL time (FApp delta is unaffected by global dilation) and restore when done.
	if (bHitStopActive)
	{
		HitStopRealLeft -= FApp::GetDeltaTime();
		if (HitStopRealLeft <= 0.f)
		{
			bHitStopActive = false;
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
		}
	}
}

void UCombatComponent::Attack(const FInputActionValue& /*Value*/)
{
	UWorld* World = GetWorld();
	UCameraComponent* Camera = OwnerChar ? OwnerChar->GetFirstPersonCamera() : nullptr;
	if (!OwnerChar || OwnerChar->IsDead() || !World || !Camera)
	{
		return;
	}

	UStatsComponent* Stats = OwnerChar->GetStatsComponent();
	UResourceComponent* Stamina = OwnerChar->GetStaminaComponent();
	UResourceComponent* Mana = OwnerChar->GetManaComponent();
	USkillTreeComponent* SkillTree = OwnerChar->GetSkillTreeComponent();
	USkeletalMeshComponent* HeldMesh = OwnerChar->GetHeldWeaponMesh();

	// Skill-tree combat modifiers (attack speed, multishot, cost reduction, lifesteal).
	const FSkillModifiers Mods = SkillTree ? SkillTree->GetModifiers() : FSkillModifiers();

	const float Now = World->GetTimeSeconds();
	const float EffectiveCooldown = AttackCooldown / (1.f + FMath::Max(0.f, Mods.AttackSpeedPct));
	if (Now - LastAttackTime < EffectiveCooldown)
	{
		return;
	}

	switch (CurrentStyle)
	{
	case ECombatStyle::Ranged:
		// Crossbow bolt: costs stamina (reducible), scales with Dexterity, may fire extra bolts.
		if (Stamina && Stamina->Spend(RangedStaminaCost * (1.f - FMath::Clamp(Mods.StaminaCostPct, 0.f, 0.8f))))
		{
			LastAttackTime = Now;
			if (HeldMesh && CrossbowShootAnim.Get()) { HeldMesh->PlayAnimation(CrossbowShootAnim.Get(), false); }
			FireProjectile(ProjectileDamage * (Stats ? Stats->GetRangedDamageMult() : 1.f), Mods.ExtraProjectiles);
		}
		else { OwnerChar->FlagStaminaDenied(); }
		break;

	case ECombatStyle::Mage:
		// Spell bolt: costs mana (reducible), scales with Intelligence, may fire extra bolts.
		if (Mana && Mana->Spend(SpellManaCost * (1.f - FMath::Clamp(Mods.ManaCostPct, 0.f, 0.8f))))
		{
			LastAttackTime = Now;
			if (HeldMesh && StaffCastAnim.Get()) { HeldMesh->PlayAnimation(StaffCastAnim.Get(), false); }

			// Release the bolt on frame 9 of the cast anim so it leaves the staff in sync with the gesture
			// (falls back to an immediate cast if the clip is missing/shorter than the release frame).
			const float SpellDamage = ProjectileDamage * (Stats ? Stats->GetSpellDamageMult() : 1.f);
			const int32 ExtraBolts = Mods.ExtraProjectiles;
			float CastDelay = 0.f;
			if (UAnimSequence* CastAnim = StaffCastAnim.Get())
			{
				const double Fps = CastAnim->GetSamplingFrameRate().AsDecimal();
				const float SafeFps = (Fps > 1.0) ? (float)Fps : 30.f;
				CastDelay = FMath::Clamp(9.f / SafeFps, 0.f, CastAnim->GetPlayLength());
			}
			if (CastDelay > 0.f)
			{
				FTimerDelegate Del = FTimerDelegate::CreateUObject(this, &UCombatComponent::FireProjectile, SpellDamage, ExtraBolts, /*bFireball*/ true);
				World->GetTimerManager().SetTimer(MageCastTimer, Del, CastDelay, false);
			}
			else
			{
				FireProjectile(SpellDamage, ExtraBolts, /*bFireball*/ true);
			}
		}
		else { OwnerChar->FlagManaDenied(); }
		break;

	case ECombatStyle::Melee:
	default:
		// Melee swing: costs stamina (reducible), gating the swing when too tired — same as ranged/mage.
		if (Stamina && !Stamina->Spend(MeleeStaminaCost * (1.f - FMath::Clamp(Mods.StaminaCostPct, 0.f, 0.8f))))
		{
			OwnerChar->FlagStaminaDenied();
			break; // out of stamina: no swing this press
		}
		LastAttackTime = Now;
		MeleeAttack();
		break;
	}
}

void UCombatComponent::MeleeAttack()
{
	UWorld* World = GetWorld();
	UCameraComponent* Camera = OwnerChar ? OwnerChar->GetFirstPersonCamera() : nullptr;
	if (!World || !Camera)
	{
		return;
	}

	// Pre-check the swing so we play exactly ONE animation: if it would strike only a solid surface
	// (wall / scenery prop) with no enemy in reach, play the deflect/bounce and deal no damage; otherwise
	// play the normal attack swing and resolve the hit partway through.
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * AttackRange;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AttackRadius), Params);

	bool bEnemyInSwing = false;
	bool bSolidInSwing = false;
	FVector SolidImpact = End;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) { continue; }
		if (HitActor->IsA(AMonsterCharacter::StaticClass())) { bEnemyInSwing = true; break; }
		if (Hit.bBlockingHit && !bSolidInSwing) { bSolidInSwing = true; SolidImpact = Hit.ImpactPoint; }
	}

	if (!bEnemyInSwing && bSolidInSwing)
	{
		PlayMeleeDeflect(SolidImpact); // bounce off the wall/prop — no swing, no damage
		return;
	}

	USkeletalMeshComponent* HeldMesh = OwnerChar->GetHeldWeaponMesh();

	// L/R alternating weapons (e.g. Club): swap primary/alt each swing and record which side fired.
	UAnimSequence* Swing = nullptr;
	if (SwingAnimAlt.Get())
	{
		bLastSwingWasLeft = bNextSwingLeft;
		Swing = bNextSwingLeft ? SwingAnim.Get() : SwingAnimAlt.Get();
		bNextSwingLeft = !bNextSwingLeft;
	}
	else
	{
		Swing = SwingAnim.Get();
	}
	if (HeldMesh && Swing)
	{
		HeldMesh->PlayAnimation(Swing, /*bLooping*/ false);
	}

	// Land the blow partway through the swing (not on the first frame) so the hit reads WITH the animation.
	const float HitDelay = Swing ? FMath::Clamp(Swing->GetPlayLength() * 0.45f, 0.05f, 0.6f) : 0.18f;
	World->GetTimerManager().SetTimer(MeleeHitTimer, this, &UCombatComponent::DoMeleeHit, HitDelay, false);
}

void UCombatComponent::DoMeleeHit()
{
	UWorld* World = GetWorld();
	UCameraComponent* Camera = OwnerChar ? OwnerChar->GetFirstPersonCamera() : nullptr;
	if (!World || !Camera || OwnerChar->IsDead())
	{
		return;
	}

	UStatsComponent* Stats = OwnerChar->GetStatsComponent();
	USkillTreeComponent* SkillTree = OwnerChar->GetSkillTreeComponent();
	UHealthComponent* Health = OwnerChar->GetHealthComponent();

	// Sphere-sweep forward from the camera and damage every monster caught in the swing.
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * AttackRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AttackRadius), Params);

	const float DamagePerHit = AttackDamage * (Stats ? Stats->GetMeleeDamageMult() : 1.f);
	float TotalDealt = 0.f;

	TSet<AActor*> AlreadyHit;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor) || !HitActor->IsA(AMonsterCharacter::StaticClass()))
		{
			continue;
		}
		AlreadyHit.Add(HitActor);
		if (AMonsterCharacter* Monster = Cast<AMonsterCharacter>(HitActor))
		{
			// For L/R alternating weapons, hint the enemy which flinch animation to play.
			if (SwingAnimAlt.Get()) { Monster->SetPendingFlinchSide(bLastSwingWasLeft); }
			TotalDealt += Monster->ApplyHitDamage(DamagePerHit, OwnerChar->GetActorLocation());
		}
	}

	// Lifesteal: heal a fraction of the melee damage dealt this swing.
	const float Lifesteal = SkillTree ? SkillTree->GetModifiers().LifestealPct : 0.f;
	if (Lifesteal > 0.f && TotalDealt > 0.f && Health)
	{
		Health->Heal(TotalDealt * Lifesteal);
	}

	// Juice: a connecting swing freezes for a few frames and kicks the camera.
	if (TotalDealt > 0.f)
	{
		TriggerHitStop();
		CameraKick(1.f);
		LastHitLandedTime = World->GetTimeSeconds(); // HUD flashes the hit marker
	}
}

void UCombatComponent::PlayMeleeDeflect(const FVector& ImpactPoint)
{
	// Bounce/clang reaction off a solid surface: play the deflect anim + a small recoil kick. (Add a
	// spark/SFX at ImpactPoint later — the impact point is passed through for that.)
	USkeletalMeshComponent* HeldMesh = OwnerChar ? OwnerChar->GetHeldWeaponMesh() : nullptr;
	if (HeldMesh && DeflectAnim.Get())
	{
		HeldMesh->PlayAnimation(DeflectAnim.Get(), /*bLooping*/ false);
	}
	CameraKick(0.6f);

	if (UDungeonGameInstance* GI = OwnerChar ? Cast<UDungeonGameInstance>(OwnerChar->GetGameInstance()) : nullptr)
	{
		GI->GetStats().DeflectsOffWalls++; // persisted on the next profile save
	}
}

void UCombatComponent::FireProjectile(float Damage, int32 ExtraProjectiles, bool bFireball)
{
	UWorld* World = GetWorld();
	UCameraComponent* Camera = OwnerChar ? OwnerChar->GetFirstPersonCamera() : nullptr;
	if (!World || !Camera || !ProjectileClass)
	{
		return;
	}

	const FVector Forward = Camera->GetForwardVector();
	const FVector SpawnOrigin = Camera->GetComponentLocation();

	FActorSpawnParameters P;
	P.Owner = OwnerChar;
	P.Instigator = OwnerChar;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spread the total bolts evenly across a small horizontal arc, centered on the aim direction.
	const int32 Count = 1 + FMath::Max(0, ExtraProjectiles);
	const float SpreadDegrees = 8.f; // per-step yaw between bolts
	const float StartYaw = -SpreadDegrees * (Count - 1) * 0.5f;

	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Dir = FRotator(0.f, StartYaw + SpreadDegrees * i, 0.f).RotateVector(Forward);
		const FVector SpawnLoc = SpawnOrigin + Dir * 60.f;
		if (AProjectile* Proj = World->SpawnActor<AProjectile>(ProjectileClass, FTransform(Dir.Rotation(), SpawnLoc), P))
		{
			if (bFireball) { Proj->EnableFireballVisual(); } // mage casts only
			Proj->Launch(Dir, Damage, OwnerChar);
		}
	}

	CameraKick(0.5f); // a light recoil on firing
}

void UCombatComponent::UseAbility(const FInputActionValue& /*Value*/)
{
	UWorld* World = GetWorld();
	USkillTreeComponent* SkillTree = OwnerChar ? OwnerChar->GetSkillTreeComponent() : nullptr;
	if (!OwnerChar || OwnerChar->IsDead() || !World || !SkillTree)
	{
		return;
	}

	UStatsComponent* Stats = OwnerChar->GetStatsComponent();
	UResourceComponent* Stamina = OwnerChar->GetStaminaComponent();
	UResourceComponent* Mana = OwnerChar->GetManaComponent();
	USkeletalMeshComponent* HeldMesh = OwnerChar->GetHeldWeaponMesh();

	// The ability available depends on the active combat style.
	EActiveAbility Ability = EActiveAbility::None;
	switch (CurrentStyle)
	{
	case ECombatStyle::Melee:  Ability = EActiveAbility::Whirlwind; break;
	case ECombatStyle::Ranged: Ability = EActiveAbility::Volley;    break;
	case ECombatStyle::Mage:   Ability = EActiveAbility::Nova;      break;
	default: break;
	}
	if (!SkillTree->HasAbility(Ability))
	{
		return; // not unlocked for this style
	}

	const float Now = World->GetTimeSeconds();
	if (const float* ReadyAt = AbilityReadyTime.Find(Ability))
	{
		if (Now < *ReadyAt)
		{
			return; // still on cooldown
		}
	}

	switch (Ability)
	{
	case EActiveAbility::Whirlwind:
		if (Stamina && Stamina->Spend(25.f))
		{
			// Use the same L/R alternation as a normal swing so the anim matches the equipped weapon.
			if (HeldMesh)
			{
				UAnimSequence* WhirlAnim = (SwingAnimAlt.Get() && !bNextSwingLeft)
					? SwingAnimAlt.Get() : SwingAnim.Get();
				if (WhirlAnim) { HeldMesh->PlayAnimation(WhirlAnim, false); }
			}
			PerformAreaBurst(350.f, AttackDamage * 1.5f * (Stats ? Stats->GetMeleeDamageMult() : 1.f));
			AbilityReadyTime.Add(Ability, Now + 6.f);
		}
		break;

	case EActiveAbility::Volley:
		if (Stamina && Stamina->Spend(22.f))
		{
			if (HeldMesh && CrossbowShootAnim.Get()) { HeldMesh->PlayAnimation(CrossbowShootAnim.Get(), false); }
			FireProjectile(ProjectileDamage * (Stats ? Stats->GetRangedDamageMult() : 1.f), /*ExtraProjectiles*/ 6);
			AbilityReadyTime.Add(Ability, Now + 5.f);
		}
		break;

	case EActiveAbility::Nova:
		if (Mana && Mana->Spend(30.f))
		{
			if (HeldMesh && StaffCastAnim.Get()) { HeldMesh->PlayAnimation(StaffCastAnim.Get(), false); }
			PerformAreaBurst(400.f, ProjectileDamage * 2.f * (Stats ? Stats->GetSpellDamageMult() : 1.f));
			AbilityReadyTime.Add(Ability, Now + 7.f);
		}
		break;

	default:
		break;
	}
}

void UCombatComponent::PerformAreaBurst(float Radius, float Damage)
{
	UWorld* World = GetWorld();
	if (!World || !OwnerChar)
	{
		return;
	}
	const FVector Center = OwnerChar->GetActorLocation();
	const float RadiusSq = Radius * Radius;

	for (TActorIterator<AMonsterCharacter> It(World); It; ++It)
	{
		AMonsterCharacter* Monster = *It;
		if (!Monster || FVector::DistSquared(Monster->GetActorLocation(), Center) > RadiusSq)
		{
			continue;
		}
		Monster->ApplyHitDamage(Damage, Center);
	}

	// Graybox VFX feedback at the player.
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ADeathPoof>(ADeathPoof::StaticClass(), FTransform(Center + FVector(0.f, 0.f, 40.f)), P);
}

void UCombatComponent::TriggerHitStop(float Duration, float Dilation)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, Dilation);
		bHitStopActive = true;
		HitStopRealLeft = Duration;
	}
}

void UCombatComponent::CameraKick(float Scale)
{
	if (APlayerController* PC = OwnerChar ? Cast<APlayerController>(OwnerChar->GetController()) : nullptr)
	{
		PC->ClientStartCameraShake(UHitCameraShake::StaticClass(), Scale);
	}
}
