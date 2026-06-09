#include "BossSpawnVFX.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"

ABossSpawnVFX::ABossSpawnVFX()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	GroundLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GroundLight"));
	GroundLight->SetupAttachment(GetRootComponent());
	GroundLight->SetCastShadows(false);
	GroundLight->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	GroundLight->SetIntensity(0.f);

	Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
	Pillar->SetupAttachment(GetRootComponent());
	Pillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pillar->SetCastShadow(false);
}

void ABossSpawnVFX::Configure(float Duration, const FLinearColor& Color)
{
	Life = FMath::Max(0.3f, Duration);
	Tint = Color;
	SetLifeSpan(Life + 0.15f); // safety net even if Tick stalls
}

void ABossSpawnVFX::TintMesh(UStaticMeshComponent* Comp) const
{
	if (Comp)
	{
		if (UMaterialInstanceDynamic* MID = Comp->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Tint);
		}
	}
}

void ABossSpawnVFX::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cube = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());
	UStaticMesh* Cyl = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad());

	if (GroundLight)
	{
		GroundLight->SetLightColor(Tint);
		GroundLight->SetAttenuationRadius(GroundFlashRadius);
	}

	// Central energy pillar (thin, tall; grows + fades in Tick).
	if (Pillar)
	{
		if (Cyl) { Pillar->SetStaticMesh(Cyl); }
		Pillar->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.01f));
		Pillar->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		TintMesh(Pillar);
	}

	// Rising swirl of glowing shards, recycled up the column.
	for (int32 i = 0; i < ShardCount; ++i)
	{
		UStaticMeshComponent* S = NewObject<UStaticMeshComponent>(this);
		S->SetupAttachment(GetRootComponent());
		S->RegisterComponent();
		S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		S->SetCastShadow(false);
		if (Cube) { S->SetStaticMesh(Cube); }
		S->SetRelativeScale3D(FVector(0.1f));
		TintMesh(S);
		Shards.Add(S);

		ShardAngle.Add(FMath::FRandRange(0.f, 2.f * PI));
		ShardSpin.Add(FMath::FRandRange(3.f, 6.f) * (FMath::FRand() < 0.5f ? -1.f : 1.f));
		ShardHeight.Add(FMath::FRandRange(0.f, ColumnHeight)); // stagger so the column is full immediately
		ShardSpeed.Add(RiseSpeed * FMath::FRandRange(0.75f, 1.25f));
	}

	// One-shot debris ring kicked outward at the base.
	for (int32 i = 0; i < DebrisCount; ++i)
	{
		UStaticMeshComponent* D = NewObject<UStaticMeshComponent>(this);
		D->SetupAttachment(GetRootComponent());
		D->RegisterComponent();
		D->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		D->SetCastShadow(false);
		if (Cube) { D->SetStaticMesh(Cube); }
		D->SetRelativeScale3D(FVector(0.14f));
		D->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
		TintMesh(D);
		Debris.Add(D);

		FVector Dir = FMath::VRand();
		Dir.Z = FMath::Abs(Dir.Z) * 0.4f + 0.15f; // outward + slightly up
		DebrisVel.Add(Dir.GetSafeNormal() * FMath::FRandRange(350.f, 620.f));
	}
}

void ABossSpawnVFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float A = (Life > 0.f) ? FMath::Clamp(Elapsed / Life, 0.f, 1.f) : 1.f; // 0 -> 1 over the intro

	// Ground light: a hard flare in the first ~25%, then a long decay to nothing.
	if (GroundLight)
	{
		const float Flare = FMath::Clamp(Elapsed / (Life * 0.25f), 0.f, 1.f); // ramp up
		const float Decay = 1.f - FMath::Clamp((A - 0.25f) / 0.75f, 0.f, 1.f); // ramp down after the peak
		GroundLight->SetIntensity(GroundFlash * Flare * Decay);
	}

	// Pillar: shoots up to full height fast, holds, then thins out and fades in the last third.
	if (Pillar)
	{
		const float GrowZ = FMath::InterpEaseOut(0.01f, ColumnHeight / 100.f, FMath::Clamp(A / 0.4f, 0.f, 1.f), 2.f);
		const float Fade = 1.f - FMath::Clamp((A - 0.6f) / 0.4f, 0.f, 1.f); // thin out near the end
		const float Width = (0.18f + 0.12f * FMath::Sin(Elapsed * 18.f)) * Fade; // flicker
		Pillar->SetRelativeScale3D(FVector(FMath::Max(0.001f, Width), FMath::Max(0.001f, Width), GrowZ));
		Pillar->SetRelativeLocation(FVector(0.f, 0.f, GrowZ * 50.f)); // cylinder pivot is centered
	}

	// Shards: orbit + climb; recycle to the base when they top out so the column stays full, then all
	// fade out (shrink) over the final third.
	const float ShardFade = 1.f - FMath::Clamp((A - 0.66f) / 0.34f, 0.f, 1.f);
	for (int32 i = 0; i < Shards.Num(); ++i)
	{
		UStaticMeshComponent* S = Shards[i];
		if (!S) { continue; }

		ShardAngle[i] += ShardSpin[i] * DeltaSeconds;
		ShardHeight[i] += ShardSpeed[i] * DeltaSeconds;
		if (ShardHeight[i] > ColumnHeight) { ShardHeight[i] -= ColumnHeight; } // wrap back to the base

		// Radius tightens as the shard climbs (funnel shape).
		const float HFrac = ShardHeight[i] / FMath::Max(1.f, ColumnHeight);
		const float Radius = SwirlRadius * (1.f - 0.6f * HFrac);
		const FVector Pos(FMath::Cos(ShardAngle[i]) * Radius, FMath::Sin(ShardAngle[i]) * Radius, ShardHeight[i]);
		S->SetRelativeLocation(Pos);
		S->SetRelativeRotation(FRotator(ShardAngle[i] * 60.f, ShardAngle[i] * 90.f, 0.f));
		S->SetRelativeScale3D(FVector((0.07f + 0.05f * (1.f - HFrac)) * ShardFade));
	}

	// Debris: fly out, decelerate, shrink away over the first ~40% (a quick kick).
	const float DebrisA = FMath::Clamp(Elapsed / (Life * 0.4f), 0.f, 1.f);
	for (int32 i = 0; i < Debris.Num(); ++i)
	{
		if (UStaticMeshComponent* D = Debris[i])
		{
			D->AddRelativeLocation(DebrisVel[i] * DeltaSeconds);
			DebrisVel[i] *= 1.f - 3.f * DeltaSeconds;          // drag
			DebrisVel[i].Z -= 900.f * DeltaSeconds;            // gravity
			D->SetRelativeScale3D(FVector(0.14f * (1.f - DebrisA)));
		}
	}

	if (Elapsed >= Life)
	{
		Destroy();
	}
}
