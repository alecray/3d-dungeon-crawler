#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"
#include "StatsComponent.h"
#include "InventoryComponent.h"
#include "MonsterCharacter.h"
#include "LootChest.h"
#include "DungeonGameInstance.h"
#include "DungeonPlayerController.h"

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
#include "CollisionShape.h"
#include "TimerManager.h"

// Skeletal sword + its swing animation are loaded from these paths if not assigned in the editor.
static const TCHAR* SwordSkeletalPath = TEXT("/Game/Weapons/Sword/SK_Sword.SK_Sword");
static const TCHAR* SwordSwingAnimPath = TEXT("/Game/Weapons/Sword/A_Sword_Swing.A_Sword_Swing");

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
}

void AFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Resolve the sword skeletal mesh + swing animation (editor assignment wins; else load by path).
	if (!SwordSkeletalAsset)
	{
		SwordSkeletalAsset = Cast<USkeletalMesh>(FSoftObjectPath(SwordSkeletalPath).TryLoad());
	}
	if (!SwingAnim)
	{
		SwingAnim = Cast<UAnimSequence>(FSoftObjectPath(SwordSwingAnimPath).TryLoad());
	}
	if (SwordMesh)
	{
		SwordMesh->SetSkeletalMesh(SwordSkeletalAsset);
		SwordMesh->SetVisibility(SwordSkeletalAsset != nullptr);
	}

	if (Health)
	{
		Health->OnDepleted.AddUObject(this, &AFirstPersonCharacter::HandleDeath);
	}

	// Load the persistent profile (attributes/level/gold) into the stats component, then size the
	// resources off the resulting stats. Auto-save whenever stats subsequently change.
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->ApplyToStats(Stats);
		GI->ApplyInventory(Inventory);
		if (GI->HasProfile())
		{
			Gold = GI->GetProfile().Gold;
		}
	}
	if (Stats)
	{
		Stats->OnStatsChanged.AddUObject(this, &AFirstPersonCharacter::HandleStatsChanged);
	}
	if (Inventory)
	{
		Inventory->OnInventoryChanged.AddUObject(this, &AFirstPersonCharacter::HandleInventoryChanged);
		Inventory->OnItemDiscovered.AddUObject(this, &AFirstPersonCharacter::HandleItemDiscovered);
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
}

void AFirstPersonCharacter::PersistProfile()
{
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		GI->CaptureFromStats(Stats, Gold);
		GI->CaptureInventory(Inventory);
		GI->SaveProfile();
	}
}

void AFirstPersonCharacter::HandleInventoryChanged(UInventoryComponent* /*ChangedInventory*/)
{
	PersistProfile();
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

	// If a loot pane is already open, E closes it.
	if (PC && PC->IsLootMenuOpen())
	{
		PC->CloseLootMenu();
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
			if (PC)
			{
				PC->OpenLootMenu(Chest);
			}
		}
	}
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

void AFirstPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Speed2D = GetVelocity().Size2D();
	const bool bWalking = Movement && !Movement->IsFalling() && Speed2D > 10.f;

	// Sprint: drain stamina while held + moving on the ground; stop when it runs out.
	const bool bSprinting = bSprintHeld && bWalking && Stamina && Stamina->GetCurrent() > 1.f;
	if (Stamina)
	{
		Stamina->SetRegenPaused(bSprinting);
		if (bSprinting)
		{
			Stamina->Drain(SprintStaminaPerSecond * DeltaSeconds);
		}
	}
	if (Movement)
	{
		Movement->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
	}

	const float MaxSpeed = (Movement && Movement->MaxWalkSpeed > 1.f) ? Movement->MaxWalkSpeed : WalkSpeed;

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

	SprintAction = NewObject<UInputAction>(this, TEXT("SprintAction"));
	SprintAction->ValueType = EInputActionValueType::Boolean;

	InteractAction = NewObject<UInputAction>(this, TEXT("InteractAction"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	InventoryToggleAction = NewObject<UInputAction>(this, TEXT("InventoryToggleAction"));
	InventoryToggleAction->ValueType = EInputActionValueType::Boolean;

	CollectionToggleAction = NewObject<UInputAction>(this, TEXT("CollectionToggleAction"));
	CollectionToggleAction->ValueType = EInputActionValueType::Boolean;

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

	// Sprint (Left Shift).
	DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);

	// Interact (E), Inventory (I), Collection log (C).
	DefaultMappingContext->MapKey(InteractAction, EKeys::E);
	DefaultMappingContext->MapKey(InventoryToggleAction, EKeys::I);
	DefaultMappingContext->MapKey(CollectionToggleAction, EKeys::C);
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
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::StartSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFirstPersonCharacter::StopSprint);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::Interact);
		EIC->BindAction(InventoryToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleInventory);
		EIC->BindAction(CollectionToggleAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleCollectionLog);
	}
}

void AFirstPersonCharacter::StartSprint(const FInputActionValue& /*Value*/)
{
	bSprintHeld = true;
}

void AFirstPersonCharacter::StopSprint(const FInputActionValue& /*Value*/)
{
	bSprintHeld = false;
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

	const float Now = World->GetTimeSeconds();
	if (Now - LastAttackTime < AttackCooldown)
	{
		return;
	}
	LastAttackTime = Now;

	// Play the Blender-authored swing animation on the sword.
	if (SwordMesh && SwingAnim)
	{
		SwordMesh->PlayAnimation(SwingAnim, /*bLooping*/ false);
	}

	// Sphere-sweep forward from the camera and damage every monster caught in the swing.
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * AttackRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AttackRadius), Params);

	TSet<AActor*> AlreadyHit;
	bool bHitSomething = false;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor) || !HitActor->IsA(AMonsterCharacter::StaticClass()))
		{
			continue;
		}
		AlreadyHit.Add(HitActor);

		if (UHealthComponent* MonsterHealth = HitActor->FindComponentByClass<UHealthComponent>())
		{
			const float Damage = AttackDamage * (Stats ? Stats->GetMeleeDamageMult() : 1.f);
			MonsterHealth->ApplyDamage(Damage);
			bHitSomething = true;
		}
	}

	if (bHitSomething && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 2, 0.6f, FColor::Yellow, TEXT("Hit!"));
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
				UGameplayStatics::OpenLevel(W, FName(*LevelName));
			}
		});
		FTimerHandle ReopenHandle;
		World->GetTimerManager().SetTimer(ReopenHandle, Reopen, 1.2f, false);
	}
}
