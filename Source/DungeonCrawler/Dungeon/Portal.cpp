#include "Portal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	struct FRingCfg
	{
		int32 Count;
		float Radius;
		float SpinDegPerSec; // orbital spin speed
		float ZAmplitude;    // helix wave height (units)
		float ZFreq;         // helix wave speed (radians/sec)
		float ShardLenScale; // local Z scale (the shard's long axis)
		float Brightness;    // tint multiplier — outer rings fade
	};

	// Inner ring spins fastest and glows brightest; outer ring drifts slowly and fades out.
	constexpr FRingCfg RingCfgs[3] = {
		{ 12,  78.f, 90.f, 22.f, 2.5f, 0.30f, 1.00f },
		{ 16, 108.f, 60.f, 14.f, 2.0f, 0.22f, 0.65f },
		{  8, 132.f, 30.f,  8.f, 1.5f, 0.16f, 0.40f },
	};
}

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UStaticMesh* Cylinder = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad());
	UStaticMesh* Cube     = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());

	// Portal surface: thin disc (flattened cylinder) stood vertical so it faces along +X. Blocks only
	// the Visibility channel so the interact line-trace can target it without impeding movement.
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(Root);
	Disc->SetStaticMesh(Cylinder);
	Disc->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
	Disc->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector, FVector::ForwardVector).Rotator());
	Disc->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.06f));
	Disc->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Disc->SetCollisionResponseToAllChannels(ECR_Ignore);
	Disc->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Build three shard rings. Shards attach directly to Root; Tick drives their position each frame
	// so each can carry an independent angle-offset Z wave (helix).
	TArray<TObjectPtr<UStaticMeshComponent>>* RingArrays[3] = { &Ring0, &Ring1, &Ring2 };
	for (int32 R = 0; R < 3; ++R)
	{
		const FRingCfg& Cfg = RingCfgs[R];
		for (int32 i = 0; i < Cfg.Count; ++i)
		{
			const float Angle = (2.f * PI * i) / Cfg.Count;
			UStaticMeshComponent* Shard = CreateDefaultSubobject<UStaticMeshComponent>(
				*FString::Printf(TEXT("R%dShard%d"), R, i));
			Shard->SetupAttachment(Root);
			Shard->SetStaticMesh(Cube);
			Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Shard->SetRelativeLocation(FVector(0.f, Cfg.Radius * FMath::Cos(Angle), CenterZ + Cfg.Radius * FMath::Sin(Angle)));
			Shard->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(Angle), 0.f, 0.f));
			Shard->SetRelativeScale3D(FVector(0.07f, 0.07f, Cfg.ShardLenScale));
			RingArrays[R]->Add(Shard);
		}
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
	Glow->SetLightColor(FLinearColor(0.25f, 0.85f, 1.0f)); // overridden in BeginPlay from TintColor
	Glow->SetIntensity(BaseIntensity);
	Glow->SetAttenuationRadius(600.f);
	Glow->SetCastShadows(false);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();

	Glow->SetLightColor(TintColor);
	ApplyGlowMaterial(Disc, 0.9f);

	TArray<TObjectPtr<UStaticMeshComponent>>* RingArrays[3] = { &Ring0, &Ring1, &Ring2 };
	for (int32 R = 0; R < 3; ++R)
	{
		for (UStaticMeshComponent* Shard : *RingArrays[R])
		{
			ApplyGlowMaterial(Shard, RingCfgs[R].Brightness);
		}
	}

	SetActive(bActive);
}

void APortal::ApplyGlowMaterial(UStaticMeshComponent* Comp, float BrightnessMult)
{
	if (!Comp)
	{
		return;
	}
	if (UMaterialInterface* Base = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")).TryLoad()))
	{
		if (UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0, Base))
		{
			MID->SetVectorParameterValue(TEXT("Color"), TintColor * BrightnessMult);
		}
	}
}

void APortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}

	Elapsed += DeltaSeconds;

	// Per-ring: spin each shard along its orbit and apply a helix Z wave offset by base angle.
	// The phase offset (BaseAngle * 2) means shards distributed around the ring form 2 helix turns,
	// giving the vortex silhouette. The wave scrolls as time passes, creating flowing motion.
	TArray<TObjectPtr<UStaticMeshComponent>>* RingArrays[3] = { &Ring0, &Ring1, &Ring2 };
	for (int32 R = 0; R < 3; ++R)
	{
		const FRingCfg& Cfg = RingCfgs[R];
		const float SpinRad = FMath::DegreesToRadians(Elapsed * Cfg.SpinDegPerSec * SwirlySpeed);
		const int32 Count   = RingArrays[R]->Num();
		for (int32 i = 0; i < Count; ++i)
		{
			UStaticMeshComponent* Shard = (*RingArrays[R])[i];
			if (!Shard) continue;

			const float BaseAngle = (2.f * PI * i) / Cfg.Count;
			const float CurAngle  = BaseAngle + SpinRad;

			const float Y      = Cfg.Radius * FMath::Cos(CurAngle);
			const float ZOrbit = Cfg.Radius * FMath::Sin(CurAngle);
			const float ZWave  = Cfg.ZAmplitude * FMath::Sin(Elapsed * Cfg.ZFreq * SwirlySpeed + BaseAngle * 2.f);

			Shard->SetRelativeLocation(FVector(0.f, Y, CenterZ + ZOrbit + ZWave));
			Shard->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(CurAngle), 0.f, 0.f));
		}
	}

	// Multi-frequency pulse gives a more energetic, "living" feel vs. a single sine.
	const float Pulse = 0.75f + 0.15f * FMath::Sin(Elapsed * 3.f) + 0.10f * FMath::Sin(Elapsed * 7.3f);
	Glow->SetIntensity(BaseIntensity * Pulse);
}

void APortal::SetActive(bool bInActive)
{
	bActive = bInActive;
	SetActorHiddenInGame(!bActive);
	if (Disc)
	{
		Disc->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (Glow)
	{
		Glow->SetVisibility(bActive);
	}
}
