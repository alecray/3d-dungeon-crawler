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

	// Light for shading the icon (local point light, won't reach the dungeon far above).
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Stage, TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(160.f, 140.f, 220.f));
	Light->SetIntensityUnits(ELightUnits::Unitless);
	Light->SetIntensity(8.f);
	Light->SetAttenuationRadius(1200.f);
	Light->RegisterComponent();

	Capture = NewObject<USceneCaptureComponent2D>(Stage, TEXT("Capture"));
	Capture->SetupAttachment(Root);
	// Final color so r.PostProcessing.PropagateAlpha gives a transparent background (empty pixels
	// keep alpha 0); the slot color then shows behind the item silhouette.
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->FOVAngle = 30.f;
	Capture->ShowOnlyActors.Add(Stage);

	// Lock exposure, kill bloom, and drop the noisy GI/temporal features so the single-frame capture
	// is clean (not dark & grainy).
	FPostProcessSettings& PP = Capture->PostProcessSettings;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.AutoExposureMinBrightness = 1.f;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.AutoExposureMaxBrightness = 1.f;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.f;

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
