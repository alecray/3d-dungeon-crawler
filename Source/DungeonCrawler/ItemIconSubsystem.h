#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemIconSubsystem.generated.h"

class UTexture2D;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPointLightComponent;

/**
 * Item icons as baked 2D thumbnails. On first request an item's mesh is rendered once in a tiny
 * off-screen studio, the pixels are processed on the CPU (linear->sRGB color + a depth-derived straight
 * alpha so the background is truly transparent and the model isn't washed out), saved as a PNG under
 * Content/Images/ItemIcons/, and returned as a UTexture2D. Subsequent runs just load the baked PNG.
 * Items with no icon mesh return null (the slot falls back to its rarity color).
 */
UCLASS()
class DUNGEONCRAWLER_API UItemIconSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Returns the icon texture for an item: cached -> baked PNG on disk -> freshly rendered+baked. */
	UTexture2D* GetIcon(FName ItemId);

	/** Drop cached icons (and optionally delete the baked PNGs) so they re-render — used by `dc.IconRebuild`. */
	void ClearCache(bool bDeleteBakedFiles);

	virtual void Deinitialize() override;

private:
	void EnsureStage();

	/** Render the item's mesh and return processed sRGB+straight-alpha pixels (BGRA). Empty on failure. */
	bool RenderIconPixels(FName ItemId, TArray<FColor>& OutPixels);

	UPROPERTY() TObjectPtr<AActor> Stage;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> StaticIcon;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> SkeletalIcon;
	UPROPERTY() TObjectPtr<UPointLightComponent> KeyLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> FillLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> HeadLight; // at the camera, lights whatever's visible (no black faces)

	UPROPERTY() TMap<FName, TObjectPtr<UTexture2D>> Cache;

	static constexpr int32 IconSize = 256;
};
