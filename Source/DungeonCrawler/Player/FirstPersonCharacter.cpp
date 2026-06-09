#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"
#include "StatsComponent.h"
#include "InventoryComponent.h"
#include "HotbarComponent.h"
#include "SkillTreeComponent.h"
#include "EquipmentComponent.h"
#include "FishingComponent.h"
#include "CombatComponent.h"
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
#include "MonsterSpawnPedestal.h"
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
	Stamina->SetRegenPerSecond(45.f); // fast recovery — fills ~144 stamina in ~3s (Dark Souls pacing)
	Stamina->SetRegenDelay(0.3f);     // short delay so brief pauses between swings actually recover
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	Hotbar = CreateDefaultSubobject<UHotbarComponent>(TEXT("Hotbar"));
	SkillTree = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTree"));
	Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));
	Fishing = CreateDefaultSubobject<UFishingComponent>(TEXT("Fishing"));
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
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

	// Force-load the weapon/tool assets up front so the .Get() at each use site below is valid. Each soft
	// ref defaults to its conventional content path (set in the header) and can be overridden per-instance
	// in the editor; LoadSynchronous returns null for an unset/unauthored clip, which the call sites no-op.
	// Weapon meshes for the held-mesh swap (attack anims live on the combat component now).
	SwordSkeletalAsset.LoadSynchronous();
	CrossbowSkeletalAsset.LoadSynchronous();
	StaffSkeletalAsset.LoadSynchronous();

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

int32 AFirstPersonCharacter::GetGold() const
{
	const UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance());
	return GI ? GI->GetGold() : 0;
}

void AFirstPersonCharacter::AddGold(int32 Amount)
{
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->GrantGold(Amount); // updates gold + the GoldLooted stat
	}
	PersistProfile(); // captures the rest of the live state and writes to disk
}

bool AFirstPersonCharacter::TrySpendGold(int32 Amount)
{
	UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance());
	if (!GI || !GI->SpendGold(Amount))
	{
		return false;
	}
	PersistProfile();
	return true;
}

void AFirstPersonCharacter::PersistProfile()
{
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->CaptureFromStats(Stats);
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

	ECombatStyle Style = ECombatStyle::Melee;
	switch (Kind)
	{
	case EEquipKind::Sword:
	{
		// Use the weapon's own mesh if it declares one; otherwise fall back to the shared sword asset.
		USkeletalMesh* WeaponMesh = SwordSkeletalAsset.Get();
		TSoftObjectPtr<UAnimSequence> PrimaryAnim;
		TSoftObjectPtr<UAnimSequence> AltAnim;
		if (ItemDatabase::Contains(ItemId))
		{
			const FItemDef& Def = ItemDatabase::Get(ItemId);
			if (!Def.WeaponMeshPath.IsNull())
			{
				WeaponMesh = Def.WeaponMeshPath.LoadSynchronous();
			}
			PrimaryAnim = Def.WeaponSwingAnimPath;
			AltAnim     = Def.WeaponSwingAnimAltPath;
		}
		SwordMesh->SetSkeletalMesh(WeaponMesh);
		SwordMesh->SetVisibility(WeaponMesh != nullptr);
		Style = ECombatStyle::Melee;
		if (Combat) { Combat->SetMeleeAnims(PrimaryAnim, AltAnim); }
		break;
	}
	case EEquipKind::Crossbow:
		SwordMesh->SetSkeletalMesh(CrossbowSkeletalAsset.Get());
		SwordMesh->SetVisibility(CrossbowSkeletalAsset.Get() != nullptr);
		Style = ECombatStyle::Ranged;
		break;
	case EEquipKind::Staff:
		SwordMesh->SetSkeletalMesh(StaffSkeletalAsset.Get());
		SwordMesh->SetVisibility(StaffSkeletalAsset.Get() != nullptr); // graybox: hidden until a staff mesh exists
		Style = ECombatStyle::Mage;
		break;
	default:
		SwordMesh->SetVisibility(false); // nothing equippable in the active slot
		Style = ECombatStyle::Melee;
		break;
	}
	if (Combat) { Combat->SetStyle(Style); }
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

	// If a loot pane, shop, or map-select menu is already open, E closes it.
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
	if (PC && PC->IsSpawnMenuOpen())
	{
		PC->CloseSpawnMenu();
		return;
	}
	if (PC && PC->IsMapSelectMenuOpen())
	{
		PC->CloseMapSelectMenu();
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
		else if (AMonsterSpawnPedestal* Pedestal = Cast<AMonsterSpawnPedestal>(Hit.GetActor()))
		{
			if (PC)
			{
				PC->OpenSpawnMenu(Pedestal);
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
				if (PC)
				{
					// Open the map + tier select menu; actual travel happens when the player clicks Go.
					PC->OpenMapSelectMenu(Portal);
				}
				else
				{
					// Fallback (no controller): immediate travel, original behaviour.
					SaveNow();
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
		if (PC->IsLootMenuOpen() || PC->IsShopOpen() || PC->IsSpawnMenuOpen() || PC->IsMapSelectMenuOpen())
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
		if (Cast<AMonsterSpawnPedestal>(Hit.GetActor()))
		{
			return TEXT("Spawn Monster");
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

double AFirstPersonCharacter::GetLastHitLandedTime() const
{
	return Combat ? Combat->GetLastHitLandedTime() : -1.0;
}

void AFirstPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Speed2D = GetVelocity().Size2D();
	const bool bWalking = Movement && !Movement->IsFalling() && Speed2D > 10.f;

	// Dash: tick the cooldown, and while a dash burst is active keep speed high + friction cut so it
	// glides the full distance; when it ends, restore normal walking.
	if (DashCooldownLeft > 0.f) { DashCooldownLeft = FMath::Max(0.f, DashCooldownLeft - DeltaSeconds); }
	// End the dash i-frame window once it elapses (only clears the invulnerability we ourselves set).
	if (bDashInvuln)
	{
		DashIFrameLeft -= DeltaSeconds;
		if (DashIFrameLeft <= 0.f)
		{
			bDashInvuln = false;
			if (Health) { Health->SetInvulnerable(false); }
		}
	}
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
					// Bigger/bushier than before so it's actually visible at the feet in first person,
					// but still flash-free and low so it stays a subtle puff.
					Dust->Configure(/*Count*/ 7, /*Speed*/ 95.f, /*BitScale*/ 0.11f, /*Life*/ 0.6f,
						/*FlashIntensity*/ 0.f, /*FlashRadius*/ 0.f); // no light — just a low puff
					Dust->SetColor(FLinearColor(0.46f, 0.42f, 0.36f)); // light dusty brown (reads on dark floors)
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
		EIC->BindAction(AttackAction, ETriggerEvent::Started, Combat.Get(), &UCombatComponent::Attack);
		EIC->BindAction(DashAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::Dash);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::Interact);
		EIC->BindAction(InventoryToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleInventory);
		EIC->BindAction(CollectionToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleCollectionLog);
		EIC->BindAction(SkillTreeToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleSkillTree);
		EIC->BindAction(AbilityAction, ETriggerEvent::Started, Combat.Get(), &UCombatComponent::UseAbility);
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

	// Dash i-frames: brief invulnerability at the start of the dodge so a well-timed dash rolls THROUGH a
	// hit (Souls-style), not just out of its way. Skip if already invulnerable (god mode) so Tick's cleanup
	// won't stomp it back off.
	if (Health && DashIFrameDuration > 0.f && !Health->IsInvulnerable())
	{
		Health->SetInvulnerable(true);
		bDashInvuln = true;
		DashIFrameLeft = DashIFrameDuration;
	}
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

void AFirstPersonCharacter::SetNoClip(bool bEnabled)
{
	bNoClip = bEnabled;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(bNoClip ? MOVE_Flying : MOVE_Walking);
		Move->MaxFlySpeed = 1200.f;
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(bNoClip ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
}

int32 AFirstPersonCharacter::GetDeathGoldCost() const
{
	const int32 Level = Stats ? Stats->GetLevel() : 1;
	// Soft (sub-linear) scaling: base cost at level 1, growing on a sqrt curve so it never spikes.
	return DeathGoldBaseCost + FMath::RoundToInt(DeathGoldLevelScale * FMath::Sqrt((float)FMath::Max(0, Level - 1)));
}

void AFirstPersonCharacter::HandleDeath(UHealthComponent* /*DeadComponent*/)
{
	if (bDead)
	{
		return;
	}

	// Death scroll: if the player is carrying one, consume it to cheat death instead of dying — revive at
	// half health / mana / stamina and carry on (no gold toll, no trip to town).
	if (Inventory && Inventory->RemoveItem(FName(TEXT("DeathScroll")), 1))
	{
		if (Health) { Health->Revive(Health->GetMaxHealth() * 0.5f); }
		if (Mana) { Mana->SetCurrent(Mana->GetMax() * 0.5f); }
		if (Stamina) { Stamina->SetCurrent(Stamina->GetMax() * 0.5f); }
		if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance())) { GI->GetStats().DeathScrollsUsed++; }
		PersistProfile(); // persist the consumed scroll + restored state
		return; // cheated death — not dead
	}

	bDead = true;

	// Tally the death and pay the death toll (gold, scaling softly with level). AddGold persists the
	// profile, so the death count + reduced gold + the rest of the state all survive the trip to town.
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->GetStats().Deaths++;
	}
	AddGold(-GetDeathGoldCost()); // clamped at 0; persists

	// Freeze the player and hand control back, then send them back to town (not a fresh dungeon).
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
			if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetController()))
			{
				PC->FadeToBlackAndTravel(TEXT("L_Town")); // dead -> fade out, back to town
			}
			else if (UWorld* W = GetWorld())
			{
				UGameplayStatics::OpenLevel(W, TEXT("L_Town"));
			}
		});
		FTimerHandle ReopenHandle;
		World->GetTimerManager().SetTimer(ReopenHandle, Reopen, 1.2f, false);
	}
}
