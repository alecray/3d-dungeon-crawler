#include "Bonfire.h"
#include "FirstPersonCharacter.h"
#include "HealthComponent.h"
#include "ResourceComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

ABonfire::ABonfire()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UStaticMesh* Cube = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded()) { Cube = CubeFinder.Object; }

	// Stone base — blocks the player + is the interact line-trace target (blocks Visibility).
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Root);
	if (Cube) { Base->SetStaticMesh(Cube); }
	Base->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.35f));
	Base->SetRelativeLocation(FVector(0.f, 0.f, 17.5f));
	Base->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Base->SetCollisionProfileName(TEXT("BlockAll"));

	// Graybox flame above the base (recolored in BeginPlay); no collision.
	Flame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Root);
	if (Cube) { Flame->SetStaticMesh(Cube); }
	Flame->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.55f));
	Flame->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
	Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Light->SetLightColor(FLinearColor(1.0f, 0.55f, 0.18f)); // warm amber
	Light->SetAttenuationRadius(900.f);
	Light->SetIntensity(BaseLightIntensity);
	Light->SetCastShadows(false);
}

void ABonfire::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMeshOverride && Base) { Base->SetStaticMesh(BaseMeshOverride); }
	if (BowlMeshOverride && Flame) { Flame->SetStaticMesh(BowlMeshOverride); }

	if (Flame)
	{
		if (UMaterialInstanceDynamic* MID = Flame->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.4f, 0.5f, 0.1f)); // hot ember
		}
	}
}

void ABonfire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Flicker the light + bob the flame a touch so it reads as a live fire.
	FlickerPhase += DeltaSeconds * 11.f;
	const float Flicker = 0.82f + 0.18f * FMath::Sin(FlickerPhase) * FMath::Sin(FlickerPhase * 2.3f + 1.f);
	if (Light) { Light->SetIntensity(BaseLightIntensity * Flicker); }
	if (Flame)
	{
		const float S = 0.55f + 0.06f * FMath::Sin(FlickerPhase * 1.7f);
		Flame->SetRelativeScale3D(FVector(0.28f, 0.28f, S));
	}
}

bool ABonfire::Rest(APawn* ByPawn)
{
	AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(ByPawn);
	if (!Player)
	{
		return false;
	}

	if (UHealthComponent* HP = Player->GetHealthComponent())
	{
		HP->Heal(HP->GetMaxHealth()); // clamps to max
	}
	if (UResourceComponent* Mana = Player->GetManaComponent())
	{
		Mana->Restore(Mana->GetMax());
	}
	if (UResourceComponent* Stamina = Player->GetStaminaComponent())
	{
		Stamina->Restore(Stamina->GetMax());
	}

	Player->SaveNow(); // bonfire doubles as a checkpoint save
	return true;
}
