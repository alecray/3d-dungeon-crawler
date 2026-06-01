#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "MonsterCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
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
	PrimaryActorTick.bCanEverTick = true; // drives the hand walking sway

	// Standard human-sized capsule.
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// First-person: the controller yaw turns the whole body; pitch/roll only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = 450.f;
	Movement->BrakingDecelerationWalking = 2000.f;
	Movement->AirControl = 0.2f;

	// ---- First-person camera at eye height ----
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // near the top of the capsule
	FirstPersonCamera->bUsePawnControlRotation = true;

	// ---- Graybox hands (engine cube), parented to the camera so they track the view ----
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	const FVector HandScale(0.125f, 0.125f, 0.125f); // ~12.5cm cubes
	const float HandForward = 45.f;
	const float HandSide = 18.f;
	const float HandDown = -22.f;

	LeftHandBase = FVector(HandForward, -HandSide, HandDown);
	RightHandBase = FVector(HandForward, HandSide, HandDown);

	LeftHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(FirstPersonCamera);
	LeftHand->SetMobility(EComponentMobility::Movable);
	LeftHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftHand->SetRelativeLocation(LeftHandBase);
	LeftHand->SetRelativeScale3D(HandScale);
	LeftHand->SetOnlyOwnerSee(true);
	if (CubeMesh.Succeeded())
	{
		LeftHand->SetStaticMesh(CubeMesh.Object);
	}

	RightHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(FirstPersonCamera);
	RightHand->SetMobility(EComponentMobility::Movable);
	RightHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightHand->SetRelativeLocation(RightHandBase);
	RightHand->SetRelativeScale3D(HandScale);
	RightHand->SetOnlyOwnerSee(true);
	if (CubeMesh.Succeeded())
	{
		RightHand->SetStaticMesh(CubeMesh.Object);
	}

	// ---- Sword held in the right hand ----
	// SwordRoot is parented to the camera (not the hand cube) so the hand's 0.125 scale doesn't
	// shrink it. The sword is modeled along +X; we pitch it up so the blade is held roughly VERTICAL
	// (pointing up) just to the lower-right of the view, leaning slightly forward. Everything
	// sword-related hangs off it so it bobs/thrusts as one piece.
	SwordBase = FVector(42.f, 24.f, -34.f);

	SwordRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SwordRoot"));
	SwordRoot->SetupAttachment(FirstPersonCamera);
	SwordRoot->SetRelativeLocation(SwordBase);
	// Pitch ~82° points the blade (+X) nearly straight up; small yaw angles it across the view.
	SwordRoot->SetRelativeRotation(FRotator(82.f, 8.f, 0.f));

	// Real-mesh sword (used only when a mesh is assigned).
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(SwordRoot);
	SwordMesh->SetMobility(EComponentMobility::Movable);
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordMesh->SetOnlyOwnerSee(true);

	// Default to an imported sword at /Game/Weapons/SM_Sword if one exists.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SwordFinder(TEXT("/Game/Weapons/SM_Sword.SM_Sword"));
	if (SwordFinder.Succeeded())
	{
		SwordMeshAsset = SwordFinder.Object;
	}

	// Graybox placeholder sword assembled from cubes (blade / crossguard / grip / pommel), built
	// along +X from the grip. Hidden automatically when a real SwordMeshAsset is present.
	auto AddSwordPart = [&](const TCHAR* Name, const FVector& Center, const FVector& SizeCm)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(SwordRoot);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetOnlyOwnerSee(true);
		Part->SetRelativeLocation(Center);
		Part->SetRelativeScale3D(SizeCm / 100.f); // engine cube is 100cm
		if (CubeMesh.Succeeded())
		{
			Part->SetStaticMesh(CubeMesh.Object);
		}
		SwordPlaceholderParts.Add(Part);
	};

	// Whole sword lies along +X (forward): pommel -> grip -> crossguard -> blade, in a straight line.
	AddSwordPart(TEXT("SwordPommel"), FVector(-16.f, 0.f, 0.f), FVector(5.f, 5.f, 5.f));
	AddSwordPart(TEXT("SwordGrip"),   FVector(-4.f, 0.f, 0.f),  FVector(18.f, 4.f, 4.f));   // along X
	AddSwordPart(TEXT("SwordGuard"),  FVector(8.f, 0.f, 0.f),   FVector(4.f, 20.f, 5.f));   // crossbar (wide in Y)
	AddSwordPart(TEXT("SwordBlade"),  FVector(46.f, 0.f, 0.f),  FVector(72.f, 4.f, 1.5f));  // long in X, flat

	// ---- Torch (floats above the left hand) ----
	TorchBase = FVector(40.f, -HandSide, 14.f); // above and slightly forward of the left hand

	TorchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchMesh"));
	TorchMesh->SetupAttachment(FirstPersonCamera);
	TorchMesh->SetMobility(EComponentMobility::Movable);
	TorchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TorchMesh->SetRelativeLocation(TorchBase);
	TorchMesh->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.12f)); // small handle-sized cube
	TorchMesh->SetOnlyOwnerSee(true);
	if (CubeMesh.Succeeded())
	{
		TorchMesh->SetStaticMesh(CubeMesh.Object);
	}

	TorchLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TorchLight"));
	TorchLight->SetupAttachment(FirstPersonCamera); // not the scaled torch cube
	TorchLight->SetRelativeLocation(TorchBase + FVector(0.f, 0.f, 20.f)); // flame floats above the handle
	TorchLight->SetIntensity(5000.f);
	TorchLight->SetAttenuationRadius(2200.f);
	TorchLight->SetLightColor(FLinearColor(1.0f, 0.72f, 0.36f)); // warm torch glow
	TorchLight->SetSourceRadius(20.f);       // soft penumbra
	TorchLight->SetSoftSourceRadius(60.f);
	TorchLight->SetCastShadows(true);
	TorchLight->SetVisibility(false); // default OFF

	// ---- Health ----
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void AFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Use the real sword mesh if one is assigned; otherwise fall back to the graybox placeholder.
	const bool bUseRealSword = (SwordMeshAsset != nullptr);
	if (SwordMesh)
	{
		SwordMesh->SetStaticMesh(SwordMeshAsset);
		SwordMesh->SetVisibility(bUseRealSword);
	}
	for (UStaticMeshComponent* Part : SwordPlaceholderParts)
	{
		if (Part)
		{
			Part->SetVisibility(!bUseRealSword);
		}
	}

	if (Health)
	{
		Health->OnDepleted.AddUObject(this, &AFirstPersonCharacter::HandleDeath);
	}
}

void AFirstPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LeftHand || !RightHand)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float MaxSpeed = (Movement && Movement->MaxWalkSpeed > 1.f) ? Movement->MaxWalkSpeed : 450.f;
	const float Speed2D = GetVelocity().Size2D();
	const bool bWalking = Movement && !Movement->IsFalling() && Speed2D > 10.f;

	// Blend the sway in while walking and out while idle/airborne.
	const float TargetWeight = bWalking ? 1.f : 0.f;
	BobWeight = FMath::FInterpTo(BobWeight, TargetWeight, DeltaSeconds, BobBlendSpeed);

	// Advance the phase proportional to current speed so faster walking = faster sway.
	const float SpeedFrac = FMath::Clamp(Speed2D / MaxSpeed, 0.f, 1.f);
	BobPhase += DeltaSeconds * (2.f * PI * BobStepRate) * SpeedFrac;
	BobPhase = FMath::Fmod(BobPhase, 2.f * PI);

	// Vertical bobs at twice the step rate (a dip per footfall); side sway at the step rate.
	const float Vert = FMath::Sin(BobPhase * 2.f) * BobVertAmplitude * BobWeight;
	const float Side = FMath::Sin(BobPhase) * BobSideAmplitude * BobWeight;

	// Forward thrust during a swing: ramps out and back over AttackThrustDuration (sine arc).
	float ThrustFwd = 0.f;
	if (AttackThrustTimeLeft > 0.f && AttackThrustDuration > 0.f)
	{
		AttackThrustTimeLeft = FMath::Max(0.f, AttackThrustTimeLeft - DeltaSeconds);
		const float Alpha = 1.f - (AttackThrustTimeLeft / AttackThrustDuration); // 0 -> 1
		ThrustFwd = FMath::Sin(Alpha * PI) * AttackThrustDistance;
	}

	const FVector Offset(ThrustFwd, Side, Vert);

	LeftHand->SetRelativeLocation(LeftHandBase + Offset);
	RightHand->SetRelativeLocation(RightHandBase + Offset);
	if (SwordRoot)
	{
		SwordRoot->SetRelativeLocation(SwordBase + Offset);
	}
	if (TorchMesh)
	{
		TorchMesh->SetRelativeLocation(TorchBase + Offset);
	}
	if (TorchLight)
	{
		TorchLight->SetRelativeLocation(TorchBase + FVector(0.f, 0.f, 20.f) + Offset);
	}

	// Lightweight on-screen health readout (prototype HUD).
	if (GEngine && Health)
	{
		const FString Msg = bDead
			? TEXT("YOU DIED — restarting...")
			: FString::Printf(TEXT("Health: %.0f / %.0f"), Health->GetHealth(), Health->GetMaxHealth());
		GEngine->AddOnScreenDebugMessage(/*Key*/ 1, 0.f, bDead ? FColor::Red : FColor::Green, Msg);
	}
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

	ToggleTorchAction = NewObject<UInputAction>(this, TEXT("ToggleTorchAction"));
	ToggleTorchAction->ValueType = EInputActionValueType::Boolean;

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

	// Toggle torch (F).
	DefaultMappingContext->MapKey(ToggleTorchAction, EKeys::F);
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
		EIC->BindAction(ToggleTorchAction, ETriggerEvent::Started, this, &AFirstPersonCharacter::ToggleTorch);
	}
}

void AFirstPersonCharacter::ToggleTorch(const FInputActionValue& /*Value*/)
{
	bTorchOn = !bTorchOn;
	if (TorchLight)
	{
		TorchLight->SetVisibility(bTorchOn);
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
	AttackThrustTimeLeft = AttackThrustDuration; // kick off the thrust animation

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
			MonsterHealth->ApplyDamage(AttackDamage);
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
