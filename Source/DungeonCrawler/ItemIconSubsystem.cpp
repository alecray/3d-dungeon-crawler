#include "ItemIconSubsystem.h"
#include "ItemTypes.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "Engine/Scene.h" // EAutoExposureMethod
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPath.h"

void UItemIconSubsystem::Deinitialize()
{
	if (Stage)
	{
		Stage->Destroy();
		Stage = nullptr;
	}
	Super::Deinitialize();
}

void UItemIconSubsystem::EnsureStage()
{
	if (Stage)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// A tiny off-screen studio far below the level.
	const FVector StageOrigin(0.f, 0.f, -100000.f);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Stage = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(StageOrigin), P);
	if (!Stage)
	{
		return;
	}

	USceneComponent* Root = NewObject<USceneComponent>(Stage, TEXT("Root"));
	Stage->SetRootComponent(Root);
	Root->RegisterComponent();

	StaticIcon = NewObject<UStaticMeshComponent>(Stage, TEXT("StaticIcon"));
	StaticIcon->SetupAttachment(Root);
	StaticIcon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticIcon->RegisterComponent();

	SkeletalIcon = NewObject<USkeletalMeshComponent>(Stage, TEXT("SkeletalIcon"));
	SkeletalIcon->SetupAttachment(Root);
	SkeletalIcon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalIcon->RegisterComponent();

	// Key + fill lights so the icon has 3D shading without one harsh blown-out side (local lights,
	// won't reach the dungeon far above).
	UPointLightComponent* Key = NewObject<UPointLightComponent>(Stage, TEXT("KeyLight"));
	Key->SetupAttachment(Root);
	Key->SetRelativeLocation(FVector(160.f, 140.f, 220.f));
	Key->SetIntensityUnits(ELightUnits::Unitless);
	Key->SetIntensity(5.f);
	Key->SetAttenuationRadius(1500.f);
	Key->RegisterComponent();

	UPointLightComponent* Fill = NewObject<UPointLightComponent>(Stage, TEXT("FillLight"));
	Fill->SetupAttachment(Root);
	Fill->SetRelativeLocation(FVector(160.f, -150.f, -40.f)); // opposite side + below to lift shadows
	Fill->SetIntensityUnits(ELightUnits::Unitless);
	Fill->SetIntensity(2.f);
	Fill->SetAttenuationRadius(1500.f);
	Fill->RegisterComponent();

	Capture = NewObject<USceneCaptureComponent2D>(Stage, TEXT("Capture"));
	Capture->SetupAttachment(Root);
	// Base color (unlit albedo): shows each item's true material colors, can't blow out from lighting,
	// and is clean (no grain). Empty pixels keep the render target's transparent clear color, so the
	// slot color shows behind the item.
	Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->FOVAngle = 30.f;
	Capture->ShowOnlyActors.Add(Stage);

	// Manual exposure (no auto/eye-adaptation) so the icon brightness is fixed and predictable — the
	// auto-exposure was over-brightening the dark studio and blowing the colors out to flat white.
	FPostProcessSettings& PP = Capture->PostProcessSettings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = AEM_Manual;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 9.f; // EV compensation: tune brightness here (higher = brighter)
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.f;

	Capture->ShowFlags.SetEyeAdaptation(false);   // no adaptation -> stable exposure
	Capture->ShowFlags.SetTemporalAA(false);
	Capture->ShowFlags.SetMotionBlur(false);
	Capture->ShowFlags.SetGlobalIllumination(false); // direct light only — avoids Lumen grain
	Capture->ShowFlags.SetScreenSpaceReflections(false);

	Capture->RegisterComponent();
}

UTextureRenderTarget2D* UItemIconSubsystem::GetIcon(FName ItemId)
{
	if (TObjectPtr<UTextureRenderTarget2D>* Found = Cache.Find(ItemId))
	{
		return *Found;
	}
	if (!ItemDatabase::Contains(ItemId))
	{
		return nullptr;
	}

	const FItemDef& Def = ItemDatabase::Get(ItemId);
	UStaticMesh* StaticMesh = !Def.IconStaticMeshPath.IsEmpty()
		? Cast<UStaticMesh>(FSoftObjectPath(Def.IconStaticMeshPath).TryLoad()) : nullptr;
	USkeletalMesh* SkelMesh = (!StaticMesh && !Def.IconSkeletalMeshPath.IsEmpty())
		? Cast<USkeletalMesh>(FSoftObjectPath(Def.IconSkeletalMeshPath).TryLoad()) : nullptr;

	if (!StaticMesh && !SkelMesh)
	{
		Cache.Add(ItemId, nullptr); // remember "no icon" so we don't retry every refresh
		return nullptr;
	}

	EnsureStage();
	if (!Stage || !Capture)
	{
		return nullptr;
	}

	// Show only the relevant mesh.
	StaticIcon->SetStaticMesh(StaticMesh);
	StaticIcon->SetVisibility(StaticMesh != nullptr);
	SkeletalIcon->SetSkeletalMesh(SkelMesh);
	SkeletalIcon->SetVisibility(SkelMesh != nullptr);

	UPrimitiveComponent* Shown = StaticMesh ? static_cast<UPrimitiveComponent*>(StaticIcon)
		: static_cast<UPrimitiveComponent*>(SkeletalIcon);
	Shown->SetRelativeLocation(FVector::ZeroVector);

	// Optional per-item recolor: bind a texture to the material's texture parameter via a MID so a
	// shared mesh/material (e.g. SK_Potion) can render health/mana/stamina variants without extra assets.
	if (!Def.IconMaterialParam.IsEmpty() && !Def.IconTexturePath.IsEmpty())
	{
		if (UTexture* Tex = Cast<UTexture>(FSoftObjectPath(Def.IconTexturePath).TryLoad()))
		{
			if (UMaterialInstanceDynamic* MID = Shown->CreateDynamicMaterialInstance(0))
			{
				MID->SetTextureParameterValue(FName(Def.IconMaterialParam), Tex);
			}
		}
	}

	// Frame the camera to the mesh bounds from a 3/4 angle. Use the largest box extent (tighter than
	// the bounding sphere) with a small margin so the item nearly fills the icon.
	const FBoxSphereBounds B = Shown->Bounds;
	const float Fit = FMath::Max(8.f, B.BoxExtent.GetMax());
	const FVector Dir = FVector(0.7f, 0.6f, 0.45f).GetSafeNormal();
	const float Dist = Fit / FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle * 0.5f)) * 1.05f;
	const FVector CamLoc = B.Origin + Dir * Dist;
	Capture->SetWorldLocationAndRotation(CamLoc, (B.Origin - CamLoc).Rotation());

	// Render target with a transparent background (alpha propagates through post-processing).
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	RT->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	RT->bAutoGenerateMips = false;
	RT->InitAutoFormat(IconSize, IconSize);
	RT->UpdateResourceImmediate(true);

	Capture->TextureTarget = RT;
	Capture->CaptureScene();

	Cache.Add(ItemId, RT);
	return RT;
}
