#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemIconSubsystem.generated.h"

class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class USkeletalMeshComponent;

/**
 * Renders each item's mesh to a small render target once and caches it, so UI slots can show a real
 * 3D thumbnail as a cheap flat texture (no per-frame scene captures). Items without an icon mesh
 * return null (the slot falls back to its rarity color).
 */
UCLASS()
class DUNGEONCRAWLER_API UItemIconSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Returns the cached icon for an item, rendering it on first request (or null if no mesh). */
	UTextureRenderTarget2D* GetIcon(FName ItemId);

	virtual void Deinitialize() override;

private:
	void EnsureStage();

	UPROPERTY() TObjectPtr<AActor> Stage;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> StaticIcon;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> SkeletalIcon;

	UPROPERTY() TMap<FName, TObjectPtr<UTextureRenderTarget2D>> Cache;

	static constexpr int32 IconSize = 256;
};
