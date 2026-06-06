#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"
#include "StatsComponent.h"
#include "InventoryComponent.h"
#include "HotbarComponent.h"
#include "SkillTreeComponent.h"
#include "EquipmentComponent.h"
#include "ItemTypes.h"
#include "MonsterCharacter.h"
#include "LootChest.h"
#include "Bonfire.h"
#include "BlackjackTable.h"
#include "FishingHole.h"
#include "CharacterClass.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"
#include "InventoryComponent.h"
#include "HotbarComponent.h"
#include "ItemPickup.h"
#include "Portal.h"
#include "ShopNPC.h"
#include "Projectile.h"
#include "DeathPoof.h"
#include "ImpactBurst.h"
#include "HitCameraShake.h"
#include "Misc/App.h"
#include "EngineUtils.h" // TActorIterator
#include "DungeonGameInstance.h"
#include "DungeonPlayerController.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "AudioDevice.h"
#include "CollisionShape.h"
#include "TimerManager.h"

// Skeletal sword + its swing animation are loaded from these paths if not assigned in the editor.
static const TCHAR* SwordSkeletalPath = TEXT("/Game/Weapons/Sword/SK_Sword.SK_Sword");
static const TCHAR* SwordSwingAnimPath = TEXT("/Game/Weapons/Sword/A_Sword_Attack.A_Sword_Attack");
static const TCHAR* SwordDeflectAnimPath = TEXT("/Game/Weapons/Sword/A_Sword_Deflect.A_Sword_Deflect");

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputActionValue.h"

AFirstPersonCharacter::AFirstPersonCharacter()
{
	PrimaryActorTick.bCanEverTick = true; // drives the sword walking sway

	// Standard human-sized capsule.
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// First-person: the controller yaw turns the whole body; pitch/roll only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->BrakingDecelerationWalking = 2000.f;
	Movement->AirControl = 0.2f;

	// ---- First-person camera at eye height ----
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // near the top of the capsule
	FirstPersonCamera->bUsePawnControlRotation = true;

	// ---- Skeletal-mesh sword held in view ----
	// Parented to the camera. A skeletal mesh lets it play Blender-authored swing animations; its
	// resting orientation comes from how it's rigged in Blender, this transform just places it in view.
	SwordBase = FVector(42.f, 24.f, -34.f);

	SwordMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(FirstPersonCamera);
	SwordMesh->SetRelativeLocation(SwordBase);
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordMesh->SetOnlyOwnerSee(true);
	// Single-node animation mode so PlayAnimation() can fire the swing on demand.
	SwordMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// ---- Stats & resources ----
	Stats = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats"));
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Mana = CreateDefaultSubobject<UResourceComponent>(TEXT("Mana"));
	Stamina = CreateDefaultSubobject<UResourceComponent>(TEXT("Stamina"));
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	Hotbar = CreateDefaultSubobject<UHotbarComponent>(TEXT("Hotbar"));
	SkillTree = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTree"));
	Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

	ProjectileClass = AProjectile::StaticClass();
}

void AFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FirstPersonCamera) { BaseFOV = FirstPersonCamera->FieldOfView; }

	// Remember the normal friction/braking so the dash can cut them for its glide and put them back.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		DefaultGroundFriction = Movement->GroundFriction;
		DefaultBrakingWalk = Movement->BrakingDecelerationWalking;
	}

	// Resolve the sword skeletal mesh + swing animation (editor assignment wins; else load by path).
	if (!SwordSkeletalAsset)
	{
		SwordSkeletalAsset = Cast<USkeletalMesh>(FSoftObjectPath(SwordSkeletalPath).TryLoad());
	}
	if (!SwingAnim)
	{
		SwingAnim = Cast<UAnimSequence>(FSoftObjectPath(SwordSwingAnimPath).TryLoad());
	}
	if (!DeflectAnim)
	{
		DeflectAnim = Cast<UAnimSequence>(FSoftObjectPath(SwordDeflectAnimPath).TryLoad());
	}
	if (!CrossbowSkeletalAsset)
	{
		CrossbowSkeletalAsset = Cast<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Weapons/Crossbow/SK_Crossbow.SK_Crossbow")).TryLoad());
	}
	if (!CrossbowShootAnim)
	{
		CrossbowShootAnim = Cast<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Weapons/Crossbow/A_Crossbow_Shoot.A_Crossbow_Shoot")).TryLoad());
	}
	if (!StaffSkeletalAsset)
	{
		// Conventional path; absent for now (graybox), so the mage just has no held mesh until art lands.
		StaffSkeletalAsset = Cast<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Weapons/Staff/SK_Staff.SK_Staff")).TryLoad());
	}

	if (Health)
	{
		Health->OnDepleted.AddUObject(this, &AFirstPersonCharacter::HandleDeath);
	}

	// Load the persistent profile (attributes/level/gold) into the stats component, then size the
	// resources off the resulting stats. Auto-save whenever stats subsequently change.
	bool bFreshProfile = true;
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		bFreshProfile = !GI->HasProfile();
		GI->ApplyToStats(Stats);
		GI->ApplySkills(SkillTree);     // layers skill bonuses onto stats before resources are sized
		GI->ApplyEquipment(Equipment);  // layers equipment bonuses too
		GI->ApplyInventory(Inventory);
		GI->ApplyHotbar(Hotbar);
		if (GI->HasProfile())
		{
			Gold = GI->GetProfile().Gold;
			// Apply saved settings (mouse sensitivity + master volume).
			SetLookSensitivity(GI->GetProfile().MouseSensitivity);
			if (UWorld* W = GetWorld())
			{
				if (FAudioDevice* Audio = W->GetAudioDeviceRaw())
				{
					Audio->SetTransientPrimaryVolume(GI->GetProfile().MasterVolume);
				}
			}
		}
	}

	// Testing convenience: a fresh game starts with a crossbow in inventory slot 2 (index 1).
	if (bFreshProfile && Inventory)
	{
		Inventory->AddItem(FName(TEXT("Crossbow")), 1); // lands in slot 0
		Inventory->MoveItem(0, 1);                      // shift to slot 2
	}
	if (Stats)
	{
		Stats->OnStatsChanged.AddUObject(this, &AFirstPersonCharacter::HandleStatsChanged);
		LastLevel = Stats->GetLevel();
	}
	if (Inventory)
	{
		Inventory->OnInventoryChanged.AddUObject(this, &AFirstPersonCharacter::HandleInventoryChanged);
		Inventory->OnItemDiscovered.AddUObject(this, &AFirstPersonCharacter::HandleItemDiscovered);
	}
	if (Hotbar)
	{
		// Start with a sword equipped if the action bar is empty (fresh game).
		if (Hotbar->GetSlotItem(0).IsNone())
		{
			Hotbar->SetSlot(0, FName(TEXT("Sword")));
			Hotbar->SelectSlot(0);
		}
		Hotbar->OnHotbarChanged.AddUObject(this, &AFirstPersonCharacter::HandleHotbarChanged);
		EquipActiveHotbarItem();
	}
	RefreshResourceMaxes(/*bRefill*/ true);
}

void AFirstPersonCharacter::RefreshResourceMaxes(bool bRefill)
{
	if (!Stats)
	{
		return;
	}
	if (Health)  { Health->SetMaxHealth(Stats->GetMaxHealth(), bRefill); }
	if (Mana)    { Mana->SetMax(Stats->GetMaxMana(), bRefill); }
	if (Stamina) { Stamina->SetMax(Stats->GetMaxStamina(), bRefill); }
}

void AFirstPersonCharacter::HandleStatsChanged(UStatsComponent* /*ChangedStats*/)
{
	RefreshResourceMaxes(/*bRefill*/ false);
	PersistProfile();

	// Level-up burst: a gold poof rising off the player.
	if (Stats && Stats->GetLevel() > LastLevel)
	{
		LastLevel = Stats->GetLevel();
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ADeathPoof* Poof = World->SpawnActor<ADeathPoof>(ADeathPoof::StaticClass(),
				FTransform(GetActorLocation()), P))
			{
				Poof->SetColor(FLinearColor(1.f, 0.85f, 0.2f)); // gold
			}
		}
	}
}

void AFirstPersonCharacter::AddGold(int32 Amount)
{
	Gold = FMath::Max(0, Gold + Amount);
	PersistProfile();
}

bool AFirstPersonCharacter::TrySpendGold(int32 Amount)
{
	if (Amount <= 0 || Gold < Amount)
	{
		return false;
	}
	Gold -= Amount;
	PersistProfile();
	return true;
}

void AFirstPersonCharacter::PersistProfile()
{
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->CaptureFromStats(Stats, Gold);
		GI->CaptureInventory(Inventory);
		GI->CaptureHotbar(Hotbar);
		GI->CaptureSkills(SkillTree);
		GI->CaptureEquipment(Equipment);
		GI->SaveProfile();
	}
}

void AFirstPersonCharacter::HandleInventoryChanged(UInventoryComponent* /*ChangedInventory*/)
{
	PersistProfile();
}

void AFirstPersonCharacter::HandleHotbarChanged(UHotbarComponent* /*ChangedHotbar*/)
{
	EquipActiveHotbarItem();
	PersistProfile();
}

void AFirstPersonCharacter::OnHotbarKey(const FInputActionInstance& Instance)
{
	if (!Hotbar)
	{
		return;
	}
	const int32 Index = HotbarActions.IndexOfByKey(Instance.GetSourceAction());
	if (Index != INDEX_NONE)
	{
		Hotbar->SelectSlot(Index);
	}
}

void AFirstPersonCharacter::EquipActiveHotbarItem()
{
	if (!SwordMesh || !Hotbar)
	{
		return;
	}

	const FName ItemId = Hotbar->GetActiveItem();
	const EEquipKind Kind = ItemDatabase::Contains(ItemId) ? ItemDatabase::Get(ItemId).EquipKind : EEquipKind::None;

	switch (Kind)
	{
	case EEquipKind::Sword:
		SwordMesh->SetSkeletalMesh(SwordSkeletalAsset);
		SwordMesh->SetVisibility(SwordSkeletalAsset != nullptr);
		CurrentStyle = ECombatStyle::Melee;
		break;
	case EEquipKind::Crossbow:
		SwordMesh->SetSkeletalMesh(CrossbowSkeletalAsset);
		SwordMesh->SetVisibility(CrossbowSkeletalAsset != nullptr);
		CurrentStyle = ECombatStyle::Ranged;
		break;
	case EEquipKind::Staff:
		SwordMesh->SetSkeletalMesh(StaffSkeletalAsset);
		SwordMesh->SetVisibility(StaffSkeletalAsset != nullptr); // graybox: hidden until a staff mesh exists
		CurrentStyle = ECombatStyle::Mage;
		break;
	default:
		SwordMesh->SetVisibility(false); // nothing equippable in the active slot
		CurrentStyle = ECombatStyle::Melee;
		break;
	}
}

void AFirstPersonCharacter::ApplyClassLoadout(ECharacterClass Class)
{
	const FClassLoadout L = GetClassLoadout(Class);

	if (Stats)
	{
		Stats->SetAttributes(L.Attributes); // fires OnStatsChanged -> resizes the resource maxes
	}
	// Start the run topped off at the new maxes.
	if (Health) { Health->Heal(Health->GetMaxHealth()); }
	if (Mana) { Mana->Restore(Mana->GetMax()); }
	if (Stamina) { Stamina->Restore(Stamina->GetMax()); }

	// Equip the class weapon in the first hotbar slot (sets the combat style via EquipActiveHotbarItem).
	if (!L.WeaponId.IsNone() && Hotbar)
	{
		Hotbar->SetSlot(0, L.WeaponId);
		Hotbar->SelectSlot(0);
		EquipActiveHotbarItem();
	}

	SaveNow();
}

void AFirstPersonCharacter::HandleItemDiscovered(FName ItemId)
{
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->AddDiscovered(ItemId);
		GI->SaveProfile();
	}
}

void AFirstPersonCharacter::Interact(const FInputActionValue& /*Value*/)
{
	UWorld* World = GetWorld();
	if (!World || !FirstPersonCamera)
	{
		return;
	}

	ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController());

	// If a loot pane or the shop is already open, E closes it.
	if (PC && PC->IsLootMenuOpen())
	{
		PC->CloseLootMenu();
		return;
	}
	if (PC && PC->IsShopOpen())
	{
		PC->CloseShop();
		return;
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (ALootChest* Chest = Cast<ALootChest>(Hit.GetActor()))
		{
			if (PC && Chest->IsViewerInFront(GetActorLocation()))
			{
				PC->OpenLootMenu(Chest);
			}
		}
		else if (AItemPickup* Pickup = Cast<AItemPickup>(Hit.GetActor()))
		{
			Pickup->Collect(this);
		}
		else if (AShopNPC* NPC = Cast<AShopNPC>(Hit.GetActor()))
		{
			if (PC)
			{
				PC->OpenShop(NPC);
			}
		}
		else if (ABonfire* Fire = Cast<ABonfire>(Hit.GetActor()))
		{
			Fire->Rest(this); // full heal + refill + checkpoint save
		}
		else if (ABlackjackTable* Table = Cast<ABlackjackTable>(Hit.GetActor()))
		{
			Table->Play(this); // opens the blackjack UI
		}
		else if (AFishingHole* Hole = Cast<AFishingHole>(Hit.GetActor()))
		{
			Hole->Interact(this); // cast / reel
		}
		else if (APortal* Portal = Cast<APortal>(Hit.GetActor()))
		{
			if (Portal->IsActive() && !Portal->GetTargetMapName().IsNone())
			{
				SaveNow(); // carry gold/inventory/stats across the level load
				if (PC)
				{
					PC->FadeToBlackAndTravel(Portal->GetTargetMapName());
				}
				else
				{
					UGameplayStatics::OpenLevel(this, Portal->GetTargetMapName());
				}
			}
		}
	}
}

FString AFirstPersonCharacter::GetInteractionPrompt() const
{
	UWorld* World = GetWorld();
	if (!World || !FirstPersonCamera)
	{
		return FString();
	}

	// Seated at the blackjack table: no world prompt (you leave via the on-screen Leave button).
	if (bBlackjackActive)
	{
		return FString();
	}

	// A loot pane or shop already open: E closes it.
	if (const ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController()))
	{
		if (PC->IsLootMenuOpen() || PC->IsShopOpen())
		{
			return TEXT("Close");
		}
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (const ALootChest* Chest = Cast<ALootChest>(Hit.GetActor()))
		{
			// Only prompt (and allow opening) when standing in front of the chest.
			return Chest->IsViewerInFront(GetActorLocation()) ? TEXT("Open") : FString();
		}
		if (Cast<AItemPickup>(Hit.GetActor()))
		{
			return TEXT("Pick up");
		}
		if (Cast<AShopNPC>(Hit.GetActor()))
		{
			return TEXT("Shop");
		}
		if (Cast<ABonfire>(Hit.GetActor()))
		{
			return TEXT("Rest");
		}
		if (Cast<ABlackjackTable>(Hit.GetActor()))
		{
			return TEXT("Play Blackjack");
		}
		if (const AFishingHole* Hole = Cast<AFishingHole>(Hit.GetActor()))
		{
			return Hole->GetPrompt();
		}
		if (const APortal* Portal = Cast<APortal>(Hit.GetActor()))
		{
			return Portal->IsActive() ? TEXT("Enter") : FString();
		}
	}
	return FString();
}

void AFirstPersonCharacter::ToggleInventory(const FInputActionValue& /*Value*/)
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController()))
	{
		PC->ToggleInventory();
	}
}

void AFirstPersonCharacter::ToggleCollectionLog(const FInputActionValue& /*Value*/)
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController()))
	{
		PC->ToggleCollectionLog();
	}
}

void AFirstPersonCharacter::ToggleSkillTree(const FInputActionValue& /*Value*/)
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController()))
	{
		PC->ToggleSkillTree();
	}
}

void AFirstPersonCharacter::UseAbility(const FInputActionValue& /*Value*/)
{
	UWorld* World = GetWorld();
	if (bDead || !World || !SkillTree)
	{
		return;
	}

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
			if (SwordMesh && SwingAnim) { SwordMesh->PlayAnimation(SwingAnim, false); }
			PerformAreaBurst(350.f, AttackDamage * 1.5f * (Stats ? Stats->GetMeleeDamageMult() : 1.f));
			AbilityReadyTime.Add(Ability, Now + 6.f);
		}
		break;

	case EActiveAbility::Volley:
		if (Stamina && Stamina->Spend(22.f))
		{
			if (SwordMesh && CrossbowShootAnim) { SwordMesh->PlayAnimation(CrossbowShootAnim, false); }
			FireProjectile(ProjectileDamage * (Stats ? Stats->GetRangedDamageMult() : 1.f), /*ExtraProjectiles*/ 6);
			AbilityReadyTime.Add(Ability, Now + 5.f);
		}
		break;

	case EActiveAbility::Nova:
		if (Mana && Mana->Spend(30.f))
		{
			PerformAreaBurst(400.f, ProjectileDamage * 2.f * (Stats ? Stats->GetSpellDamageMult() : 1.f));
			AbilityReadyTime.Add(Ability, Now + 7.f);
		}
		break;

	default:
		break;
	}
}

void AFirstPersonCharacter::PerformAreaBurst(float Radius, float Damage)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector Center = GetActorLocation();
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

void AFirstPersonCharacter::TriggerHitStop(float Duration, float Dilation)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, Dilation);
		bHitStopActive = true;
		HitStopRealLeft = Duration;
	}
}

void AFirstPersonCharacter::CameraKick(float Scale)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ClientStartCameraShake(UHitCameraShake::StaticClass(), Scale);
	}
}

void AFirstPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Speed2D = GetVelocity().Size2D();
	const bool bWalking = Movement && !Movement->IsFalling() && Speed2D > 10.f;

	// Dash: tick the cooldown, and while a dash burst is active keep speed high + friction cut so it
	// glides the full distance; when it ends, restore normal walking.
	if (DashCooldownLeft > 0.f) { DashCooldownLeft = FMath::Max(0.f, DashCooldownLeft - DeltaSeconds); }
	if (StaminaDenyFlashLeft > 0.f) { StaminaDenyFlashLeft = FMath::Max(0.f, StaminaDenyFlashLeft - DeltaSeconds); }
	if (ManaDenyFlashLeft > 0.f) { ManaDenyFlashLeft = FMath::Max(0.f, ManaDenyFlashLeft - DeltaSeconds); }
	const bool bDashing = DashTimeLeft > 0.f;
	if (Movement)
	{
		if (bDashing)
		{
			DashTimeLeft = FMath::Max(0.f, DashTimeLeft - DeltaSeconds);
			Movement->MaxWalkSpeed = DashSpeed;
			if (DashTimeLeft <= 0.f)
			{
				// Burst over: restore friction/braking + walk speed, and clamp the leftover dash velocity
				// down to a normal walk. Without this, holding a movement key skips braking (it only applies
				// with no input) so the 2000-speed bled off slowly and carried you way too far.
				Movement->GroundFriction = DefaultGroundFriction;
				Movement->BrakingDecelerationWalking = DefaultBrakingWalk;
				Movement->MaxWalkSpeed = WalkSpeed;

				const FVector Vel = Movement->Velocity;
				const FVector2D Horiz(Vel.X, Vel.Y);
				if (Horiz.SizeSquared() > FMath::Square(WalkSpeed))
				{
					const FVector2D Capped = Horiz.GetSafeNormal() * WalkSpeed;
					Movement->Velocity = FVector(Capped.X, Capped.Y, Vel.Z);
				}
			}
		}
		else
		{
			Movement->MaxWalkSpeed = WalkSpeed;
		}
	}

	// Game feel: ease the FOV out a touch during the dash; ramp chromatic aberration + desaturation as
	// health drops (layered on the low-health vignette). Both are near-free.
	if (FirstPersonCamera)
	{
		const float TargetFOV = BaseFOV + (bDashing ? SprintFOVKick : 0.f);
		FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaSeconds, 8.f));

		const float Pct = Health ? Health->GetHealthPercent() : 1.f;
		const float Danger = (LowHpThreshold > 0.f) ? FMath::Clamp((LowHpThreshold - Pct) / LowHpThreshold, 0.f, 1.f) : 0.f;
		FPostProcessSettings& PP = FirstPersonCamera->PostProcessSettings;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = Danger * LowHpFringe;
		PP.bOverride_ColorSaturation = true;
		const float Sat = 1.f - Danger * LowHpDesat;
		PP.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);
		FirstPersonCamera->PostProcessBlendWeight = 1.f;
	}

	const float MaxSpeed = (Movement && Movement->MaxWalkSpeed > 1.f) ? Movement->MaxWalkSpeed : WalkSpeed;

	// Footstep dust: a small puff at the feet every FootstepInterval cm walked on the ground.
	if (bWalking && !bNoClip)
	{
		StepAccum += Speed2D * DeltaSeconds;
		if (StepAccum >= FootstepInterval)
		{
			StepAccum = 0.f;
			if (UWorld* World = GetWorld())
			{
				const float CapHalf = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
				const FTransform FeetXf(GetActorLocation() - FVector(0.f, 0.f, CapHalf - 2.f));
				// Deferred so the subtle, flash-FREE dust config + tint land before BeginPlay builds it
				// (a plain SpawnActor would run BeginPlay first, leaving the default bright white flash).
				if (AImpactBurst* Dust = World->SpawnActorDeferred<AImpactBurst>(
					AImpactBurst::StaticClass(), FeetXf, this, nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
				{
					Dust->Configure(/*Count*/ 3, /*Speed*/ 60.f, /*BitScale*/ 0.045f, /*Life*/ 0.45f,
						/*FlashIntensity*/ 0.f, /*FlashRadius*/ 0.f); // no light — just a low puff
					Dust->SetColor(FLinearColor(0.32f, 0.29f, 0.25f)); // muted dusty brown
					UGameplayStatics::FinishSpawningActor(Dust, FeetXf);
				}
			}
		}
	}
	else
	{
		StepAccum = 0.f;
	}

	// Blend the sway in while walking and out while idle/airborne.
	const float TargetWeight = bWalking ? 1.f : 0.f;
	BobWeight = FMath::FInterpTo(BobWeight, TargetWeight, DeltaSeconds, BobBlendSpeed);

	// Advance the phase proportional to current speed so faster walking = faster sway.
	const float SpeedFrac = FMath::Clamp(Speed2D / MaxSpeed, 0.f, 1.f);
	BobPhase += DeltaSeconds * (2.f * PI * BobStepRate) * SpeedFrac;
	BobPhase = FMath::Fmod(BobPhase, 2.f * PI);

	// Vertical bobs at twice the step rate (a dip per footfall); side sway at the step rate. (The
	// sword swing itself is now a played animation, so there's no procedural thrust here.)
	const float Vert = FMath::Sin(BobPhase * 2.f) * BobVertAmplitude * BobWeight;
	const float Side = FMath::Sin(BobPhase) * BobSideAmplitude * BobWeight;

	const FVector Offset(0.f, Side, Vert);

	if (SwordMesh)
	{
		SwordMesh->SetRelativeLocation(SwordBase + Offset);
	}
	// (Health/mana/stamina now show on the UMG HUD via ADungeonPlayerController.)
}

void AFirstPersonCharacter::EnsureInputAssets()
{
	if (bInputAssetsBuilt)
	{
		return;
	}
	bInputAssetsBuilt = true;

	MoveAction = NewObject<UInputAction>(this, TEXT("MoveAction"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	LookAction = NewObject<UInputAction>(this, TEXT("LookAction"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	JumpAction = NewObject<UInputAction>(this, TEXT("JumpAction"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	AttackAction = NewObject<UInputAction>(this, TEXT("AttackAction"));
	AttackAction->ValueType = EInputActionValueType::Boolean;

	DashAction = NewObject<UInputAction>(this, TEXT("DashAction"));
	DashAction->ValueType = EInputActionValueType::Boolean;

	InteractAction = NewObject<UInputAction>(this, TEXT("InteractAction"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	InventoryToggleAction = NewObject<UInputAction>(this, TEXT("InventoryToggleAction"));
	InventoryToggleAction->ValueType = EInputActionValueType::Boolean;

	CollectionToggleAction = NewObject<UInputAction>(this, TEXT("CollectionToggleAction"));
	CollectionToggleAction->ValueType = EInputActionValueType::Boolean;

	SkillTreeToggleAction = NewObject<UInputAction>(this, TEXT("SkillTreeToggleAction"));
	SkillTreeToggleAction->ValueType = EInputActionValueType::Boolean;

	AbilityAction = NewObject<UInputAction>(this, TEXT("AbilityAction"));
	AbilityAction->ValueType = EInputActionValueType::Boolean;

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("DefaultMappingContext"));

	// Move (Axis2D): X = right, Y = forward. Digital keys land on X, so swizzle to reach Y.
	{
		FEnhancedActionKeyMapping& M = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
		M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this)); // X -> Y
	}
	{
		FEnhancedActionKeyMapping& M = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
		M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this)); // X -> Y
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));      // -> -Y
	}
	{
		FEnhancedActionKeyMapping& M = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));      // -> -X
	}
	DefaultMappingContext->MapKey(MoveAction, EKeys::D);                // -> +X

	// Look (mouse delta).
	DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D);

	// Jump (space bar).
	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);

	// Attack (left mouse button).
	DefaultMappingContext->MapKey(AttackAction, EKeys::LeftMouseButton);

	// Dash (Left Shift).
	DefaultMappingContext->MapKey(DashAction, EKeys::LeftShift);

	// Interact (E), Inventory (I), Collection log (C).
	DefaultMappingContext->MapKey(InteractAction, EKeys::E);
	DefaultMappingContext->MapKey(InventoryToggleAction, EKeys::I);
	DefaultMappingContext->MapKey(CollectionToggleAction, EKeys::C);
	DefaultMappingContext->MapKey(SkillTreeToggleAction, EKeys::K);
	DefaultMappingContext->MapKey(AbilityAction, EKeys::Q);

	// Hotbar select (number keys 1-8).
	static const FKey HotbarKeys[8] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
		EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
	HotbarActions.SetNum(8);
	for (int32 i = 0; i < 8; ++i)
	{
		HotbarActions[i] = NewObject<UInputAction>(this, *FString::Printf(TEXT("HotbarAction%d"), i + 1));
		HotbarActions[i]->ValueType = EInputActionValueType::Boolean;
		DefaultMappingContext->MapKey(HotbarActions[i], HotbarKeys[i]);
	}
}

void AFirstPersonCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureInputAssets();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Clamp how far the camera can pitch up/down.
		if (APlayerCameraManager* CamMgr = PC->PlayerCameraManager)
		{
			CamMgr->ViewPitchMin = CameraPitchMin;
			CamMgr->ViewPitchMax = CameraPitchMax;
		}

		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				Subsystem->ClearAllMappings();
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AFirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureInputAssets();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::Move);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::Look);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::Attack);
		EIC->BindAction(DashAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::Dash);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::Interact);
		EIC->BindAction(InventoryToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleInventory);
		EIC->BindAction(CollectionToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleCollectionLog);
		EIC->BindAction(SkillTreeToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleSkillTree);
		EIC->BindAction(AbilityAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::UseAbility);
		for (UInputAction* HotbarAct : HotbarActions)
		{
			if (HotbarAct)
			{
				EIC->BindAction(HotbarAct, ETriggerEvent::Started, this, &AFirstPersonCharacter::OnHotbarKey);
			}
		}
	}
}

void AFirstPersonCharacter::FlagStaminaDenied() { StaminaDenyFlashLeft = ResourceDenyFlashDuration; }
void AFirstPersonCharacter::FlagManaDenied()    { ManaDenyFlashLeft = ResourceDenyFlashDuration; }

void AFirstPersonCharacter::Dash(const FInputActionValue& /*Value*/)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || bDead || bNoClip)
	{
		return;
	}
	// Ground-only, respect the cooldown, and require stamina (the dash is a committed, costed move).
	if (Movement->IsFalling() || DashTimeLeft > 0.f || DashCooldownLeft > 0.f)
	{
		return;
	}
	if (!Stamina || Stamina->GetCurrent() < DashStaminaCost)
	{
		FlagStaminaDenied(); // can't dash — flash the stamina bar
		return;
	}

	// Dash where you're moving; if standing still, dash the way you're facing.
	FVector Dir = GetVelocity();
	Dir.Z = 0.f;
	if (!Dir.Normalize())
	{
		const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
		Dir = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	}
	DashDir = Dir;

	// Launch + cut friction/braking for the burst window so it glides the full distance, then restore.
	Movement->GroundFriction = 0.f;
	Movement->BrakingDecelerationWalking = 0.f;
	Movement->MaxWalkSpeed = DashSpeed;
	LaunchCharacter(DashDir * DashSpeed, /*bXYOverride*/ true, /*bZOverride*/ false);

	DashTimeLeft = DashDuration;
	DashCooldownLeft = DashCooldown;
	Stamina->Drain(DashStaminaCost);
	Stamina->SetRegenPaused(false);
}

void AFirstPersonCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero() || !Controller)
	{
		return;
	}

	// Move relative to where the controller is looking (yaw only, so looking up/down doesn't slow us).
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AFirstPersonCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(Axis.X * LookSensitivity);
		// Negate pitch so moving the mouse up looks up (positive pitch input looks down).
		AddControllerPitchInput(-Axis.Y * LookSensitivity);
	}
}

void AFirstPersonCharacter::Attack(const FInputActionValue& /*Value*/)
{
	UWorld* World = GetWorld();
	if (bDead || !World || !FirstPersonCamera)
	{
		return;
	}

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
			if (SwordMesh && CrossbowShootAnim) { SwordMesh->PlayAnimation(CrossbowShootAnim, false); }
			FireProjectile(ProjectileDamage * (Stats ? Stats->GetRangedDamageMult() : 1.f), Mods.ExtraProjectiles);
		}
		else { FlagStaminaDenied(); }
		break;

	case ECombatStyle::Mage:
		// Spell bolt: costs mana (reducible), scales with Intelligence, may fire extra bolts.
		if (Mana && Mana->Spend(SpellManaCost * (1.f - FMath::Clamp(Mods.ManaCostPct, 0.f, 0.8f))))
		{
			LastAttackTime = Now;
			FireProjectile(ProjectileDamage * (Stats ? Stats->GetSpellDamageMult() : 1.f), Mods.ExtraProjectiles);
		}
		else { FlagManaDenied(); }
		break;

	case ECombatStyle::Melee:
	default:
		// Melee swing: costs stamina (reducible), gating the swing when too tired — same as ranged/mage.
		if (Stamina && !Stamina->Spend(MeleeStaminaCost * (1.f - FMath::Clamp(Mods.StaminaCostPct, 0.f, 0.8f))))
		{
			FlagStaminaDenied();
			break; // out of stamina: no swing this press
		}
		LastAttackTime = Now;
		MeleeAttack();
		break;
	}
}

void AFirstPersonCharacter::MeleeAttack()
{
	UWorld* World = GetWorld();
	if (!World || !FirstPersonCamera)
	{
		return;
	}

	// Pre-check the swing so we play exactly ONE animation: if it would strike only a solid surface
	// (wall / scenery prop) with no enemy in reach, play the deflect/bounce and deal no damage; otherwise
	// play the normal attack swing and resolve the hit partway through.
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * AttackRange;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

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

	if (SwordMesh && SwingAnim)
	{
		SwordMesh->PlayAnimation(SwingAnim, /*bLooping*/ false);
	}

	// Land the blow partway through the swing (not on the first frame) so the hit reads WITH the animation.
	const float HitDelay = SwingAnim ? FMath::Clamp(SwingAnim->GetPlayLength() * 0.45f, 0.05f, 0.6f) : 0.18f;
	World->GetTimerManager().SetTimer(MeleeHitTimer, this, &AFirstPersonCharacter::DoMeleeHit, HitDelay, false);
}

void AFirstPersonCharacter::DoMeleeHit()
{
	UWorld* World = GetWorld();
	if (!World || !FirstPersonCamera || bDead)
	{
		return;
	}

	// Sphere-sweep forward from the camera and damage every monster caught in the swing.
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * AttackRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

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
			TotalDealt += Monster->ApplyHitDamage(DamagePerHit, GetActorLocation());
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
	}
}

void AFirstPersonCharacter::PlayMeleeDeflect(const FVector& ImpactPoint)
{
	// Bounce/clang reaction off a solid surface: play the deflect anim + a small recoil kick. (Add a
	// spark/SFX at ImpactPoint later — the impact point is passed through for that.)
	if (SwordMesh && DeflectAnim)
	{
		SwordMesh->PlayAnimation(DeflectAnim, /*bLooping*/ false);
	}
	CameraKick(0.6f);
}

void AFirstPersonCharacter::FireProjectile(float Damage, int32 ExtraProjectiles)
{
	UWorld* World = GetWorld();
	if (!World || !FirstPersonCamera || !ProjectileClass)
	{
		return;
	}

	const FVector Forward = FirstPersonCamera->GetForwardVector();
	const FVector SpawnOrigin = FirstPersonCamera->GetComponentLocation();

	FActorSpawnParameters P;
	P.Owner = this;
	P.Instigator = this;
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
			Proj->Launch(Dir, Damage, this);
		}
	}

	CameraKick(0.5f); // a light recoil on firing
}

bool AFirstPersonCharacter::DevToggleNoClip()
{
	bNoClip = !bNoClip;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(bNoClip ? MOVE_Flying : MOVE_Walking);
		Move->MaxFlySpeed = 1200.f;
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(bNoClip ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
	return bNoClip;
}

bool AFirstPersonCharacter::DevToggleGodMode()
{
	if (Health)
	{
		Health->SetInvulnerable(!Health->IsInvulnerable());
		return Health->IsInvulnerable();
	}
	return false;
}

void AFirstPersonCharacter::DevKill()
{
	if (Health)
	{
		Health->SetInvulnerable(false); // a dev kill always lands, even with god mode on
		Health->Kill();
	}
}

void AFirstPersonCharacter::HandleDeath(UHealthComponent* /*DeadComponent*/)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	// Persist progress so attributes/level/gold survive the restart.
	PersistProfile();

	// Freeze the player and hand control back, then reload the level for a fresh dungeon run.
	GetCharacterMovement()->DisableMovement();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate Reopen;
		Reopen.BindLambda([this]()
		{
			if (UWorld* W = GetWorld())
			{
				// GetCurrentLevelName strips the PIE prefix so the reload works in-editor too.
				const FString LevelName = UGameplayStatics::GetCurrentLevelName(W, /*bRemovePrefixString*/ true);
				if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController()))
				{
					PC->FadeToBlackAndTravel(FName(*LevelName)); // fade out, then reload for a fresh run
				}
				else
				{
					UGameplayStatics::OpenLevel(W, FName(*LevelName));
				}
			}
		});
		FTimerHandle ReopenHandle;
		World->GetTimerManager().SetTimer(ReopenHandle, Reopen, 1.2f, false);
	}
}
