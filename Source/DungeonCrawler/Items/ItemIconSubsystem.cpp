#include "ItemIconSubsystem.h"
#include "ItemTypes.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "TextureResource.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

// --- Live tuning: light intensities (then run `dc.IconRebuild` and reopen the inventory). ---
static TAutoConsoleVariable<float> CVarIconKey(TEXT("dc.IconKeyLight"), 1.0f,
	TEXT("Item icon key-light intensity (main brightness)."));
static TAutoConsoleVariable<float> CVarIconFill(TEXT("dc.IconFillLight"), 5.0f,
	TEXT("Item icon fill-light intensity (lifts shadows)."));
static TAutoConsoleVariable<float> CVarIconHead(TEXT("dc.IconHeadLight"), 7.0f,
	TEXT("Item icon headlight intensity (sits at the camera so visible faces are never black, e.g. metal blades)."));

static FAutoConsoleCommandWithWorld GIconRebuildCmd(
	TEXT("dc.IconRebuild"),
	TEXT("Delete baked item icons + drop the cache so they re-render with current dc.Icon* settings."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (World)
		{
			if (UItemIconSubsystem* S = World->GetSubsystem<UItemIconSubsystem>())
			{
				S->ClearCache(/*bDeleteBakedFiles*/ true);
			}
		}
	}));

static FString IconBakeDir()
{
	return FPaths::ProjectContentDir() / TEXT("Images/ItemIcons");
}

static FString IconPngPath(FName ItemId)
{
	return IconBakeDir() / (ItemId.ToString() + TEXT(".png"));
}

// Decode a baked icon PNG (BGRA) into a transient texture.
static UTexture2D* LoadPngTexture(UObject* Outer, const FString& Path)
{
	TArray<uint8> Raw;
	if (!FFileHelper::LoadFileToArray(Raw, *Path))
	{
		return nullptr;
	}
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	TArray<uint8> Bgra;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Raw.GetData(), Raw.Num()) || !Wrapper->GetRaw(ERGBFormat::BGRA, 8, Bgra))
	{
		return nullptr;
	}
	const int32 W = Wrapper->GetWidth();
	const int32 H = Wrapper->GetHeight();
	UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}
	Tex->SRGB = true;
	void* Dest = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Dest, Bgra.GetData(), Bgra.Num());
	Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
	Tex->UpdateResource();
	return Tex;
}

// Save processed BGRA pixels as a PNG (the "bake").
static void SavePngTexture(const TArray<FColor>& Pixels, int32 W, int32 H, const FString& Path)
{
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() || !Wrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), W, H, ERGBFormat::BGRA, 8))
	{
		return;
	}
	const TArray64<uint8>& Compressed = Wrapper->GetCompressed();
	IFileManager::Get().MakeDirectory(*IconBakeDir(), /*Tree*/ true);
	TArray<uint8> Out(Compressed.GetData(), (int32)Compressed.Num());
	FFileHelper::SaveArrayToFile(Out, *Path);
}

void UItemIconSubsystem::Deinitialize()
{
	if (Stage)
	{
		Stage->Destroy();
		Stage = nullptr;
	}
	Super::Deinitialize();
}

void UItemIconSubsystem::ClearCache(bool bDeleteBakedFiles)
{
	Cache.Reset();
	if (bDeleteBakedFiles)
	{
		IFileManager::Get().DeleteDirectory(*IconBakeDir(), /*RequireExists*/ false, /*Tree*/ true);
	}
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

	// Key + fill + head lights (capture-only). Positions/intensities/attenuation are set per render in
	// RenderIconPixels so they scale with each item's framed size (fixed offsets left big items unlit).
	auto MakeLight = [&](const TCHAR* Name) -> UPointLightComponent*
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(Stage, Name);
		L->SetupAttachment(Root);
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->SetCastShadows(false);
		L->bUseInverseSquaredFalloff = false; // uniform studio brightness (no harsh distance falloff)
		L->SetLightFalloffExponent(1.f);
		L->RegisterComponent();
		return L;
	};
	KeyLight = MakeLight(TEXT("KeyLight"));
	FillLight = MakeLight(TEXT("FillLight"));
	HeadLight = MakeLight(TEXT("HeadLight")); // sits at the camera so visible faces (e.g. metal blades) aren't black

	Capture = NewObject<USceneCaptureComponent2D>(Stage, TEXT("Capture"));
	Capture->SetupAttachment(Root);
	// Lit scene color (linear HDR) in RGB + scene DEPTH in alpha. We convert linear->sRGB and turn the
	// depth into a clean straight-alpha mask on the CPU — so no tonemapper washout and a truly transparent
	// background, without depending on PropagateAlpha / TemporalAA.
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorSceneDepth;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->FOVAngle = 30.f;
	Capture->ShowOnlyActors.Add(Stage);

	// Off-screen studio: kill everything global that would otherwise bleed in and wash the icon out.
	Capture->ShowFlags.SetEyeAdaptation(false);
	Capture->ShowFlags.SetTemporalAA(false);
	Capture->ShowFlags.SetMotionBlur(false);
	Capture->ShowFlags.SetGlobalIllumination(false);
	Capture->ShowFlags.SetScreenSpaceReflections(false);
	Capture->ShowFlags.SetBloom(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetVolumetricFog(false);
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetSkyLighting(false);

	Capture->RegisterComponent();
}

bool UItemIconSubsystem::RenderIconPixels(FName ItemId, TArray<FColor>& OutPixels)
{
	const FItemDef& Def = ItemDatabase::Get(ItemId);
	UStaticMesh* StaticMesh = Def.IconStaticMeshPath.IsNull()
		? nullptr : Def.IconStaticMeshPath.LoadSynchronous();
	USkeletalMesh* SkelMesh = (!StaticMesh && !Def.IconSkeletalMeshPath.IsNull())
		? Def.IconSkeletalMeshPath.LoadSynchronous() : nullptr;
	if (!StaticMesh && !SkelMesh)
	{
		return false;
	}

	EnsureStage();
	if (!Stage || !Capture)
	{
		return false;
	}

	StaticIcon->SetStaticMesh(StaticMesh);
	StaticIcon->SetVisibility(StaticMesh != nullptr);
	SkeletalIcon->SetSkeletalMesh(SkelMesh);
	SkeletalIcon->SetVisibility(SkelMesh != nullptr);

	UPrimitiveComponent* Shown = StaticMesh ? static_cast<UPrimitiveComponent*>(StaticIcon)
		: static_cast<UPrimitiveComponent*>(SkeletalIcon);
	Shown->SetRelativeLocation(FVector::ZeroVector);

	// Optional per-item recolor (e.g. potion variants share one mesh/material).
	if (!Def.IconMaterialParam.IsEmpty() && !Def.IconTexturePath.IsNull())
	{
		if (UTexture* Tex = Def.IconTexturePath.LoadSynchronous())
		{
			if (UMaterialInstanceDynamic* MID = Shown->CreateDynamicMaterialInstance(0))
			{
				MID->SetTextureParameterValue(FName(Def.IconMaterialParam), Tex);
			}
		}
	}

	// Frame the camera to the mesh bounds from a 3/4 angle.
	const FBoxSphereBounds B = Shown->Bounds;
	const float Fit = FMath::Max(8.f, B.BoxExtent.GetMax());
	const FVector Dir = FVector(0.7f, 0.6f, 0.45f).GetSafeNormal();
	// Pull the camera in so the item fills more of the icon (smaller margin = more zoom).
	const float Dist = Fit / FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle * 0.5f)) * 0.82f;
	const FVector CamLoc = B.Origin + Dir * Dist;
	Capture->SetWorldLocationAndRotation(CamLoc, (B.Origin - CamLoc).Rotation());

	// Position + tune the lights relative to THIS item's framed size (fixed offsets left big items dark).
	const float Radius = FMath::Max(300.f, Dist * 4.f);
	auto PlaceLight = [&](UPointLightComponent* L, const FVector& OffsetDir, float Intensity)
	{
		if (!L) { return; }
		L->SetWorldLocation(B.Origin + OffsetDir.GetSafeNormal() * Dist);
		L->SetIntensity(Intensity);
		L->SetAttenuationRadius(Radius);
	};
	PlaceLight(KeyLight,  FVector(0.6f, 0.5f, 0.7f),  CVarIconKey.GetValueOnGameThread());  // upper front-right
	PlaceLight(FillLight, FVector(-0.6f, -0.4f, 0.2f), CVarIconFill.GetValueOnGameThread()); // opposite, low
	// Headlight sits at the camera so every visible face is lit (no black metal blades).
	if (HeadLight)
	{
		HeadLight->SetWorldLocation(CamLoc);
		HeadLight->SetIntensity(CVarIconHead.GetValueOnGameThread());
		HeadLight->SetAttenuationRadius(Radius * 1.5f);
	}

	// Render to a float target (we read back linear HDR + depth).
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	RT->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	RT->bForceLinearGamma = true;
	RT->bAutoGenerateMips = false;
	RT->InitAutoFormat(IconSize, IconSize);
	RT->UpdateResourceImmediate(true);

	Capture->TextureTarget = RT;
	Capture->CaptureScene();

	FTextureRenderTargetResource* Res = RT->GameThread_GetRenderTargetResource();
	if (!Res)
	{
		return false;
	}
	TArray<FLinearColor> Linear;
	if (!Res->ReadLinearColorPixels(Linear) || Linear.Num() != IconSize * IconSize)
	{
		return false;
	}

	// Pixels beyond the object (background / far) are transparent; object pixels (finite, near depth) are
	// opaque. Depth lives in alpha (world units). Color is linear -> convert to sRGB.
	const float DepthCut = Dist + Fit * 2.f + 50.f;
	OutPixels.SetNumUninitialized(Linear.Num());
	for (int32 i = 0; i < Linear.Num(); ++i)
	{
		const float Depth = Linear[i].A;
		const bool bObject = (Depth > 1.f && Depth < DepthCut);
		FLinearColor C = Linear[i];
		C.A = bObject ? 1.f : 0.f;
		OutPixels[i] = C.ToFColor(/*bSRGB*/ true); // sRGB on RGB; alpha 0/1 -> 0/255
	}
	return true;
}

UTexture2D* UItemIconSubsystem::GetIcon(FName ItemId)
{
	if (TObjectPtr<UTexture2D>* Found = Cache.Find(ItemId))
	{
		return *Found;
	}
	if (!ItemDatabase::Contains(ItemId))
	{
		return nullptr;
	}

	// 1) Use a previously-baked PNG if present.
	const FString Path = IconPngPath(ItemId);
	if (IFileManager::Get().FileExists(*Path))
	{
		if (UTexture2D* Loaded = LoadPngTexture(this, Path))
		{
			Cache.Add(ItemId, Loaded);
			return Loaded;
		}
	}

	// 2) Render + process the icon, bake it to PNG, and return a texture from the processed pixels.
	TArray<FColor> Pixels;
	if (!RenderIconPixels(ItemId, Pixels))
	{
		Cache.Add(ItemId, nullptr); // no mesh / failed — don't retry every refresh
		return nullptr;
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(IconSize, IconSize, PF_B8G8R8A8);
	if (Tex)
	{
		Tex->SRGB = true;
		void* Dest = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
		Tex->UpdateResource();
	}
	SavePngTexture(Pixels, IconSize, IconSize, Path); // bake for next time

	Cache.Add(ItemId, Tex);
	return Tex;
}
