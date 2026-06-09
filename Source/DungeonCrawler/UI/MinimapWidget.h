#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

class UImage;
class UTexture2D;
class ADungeonGenerator;

/**
 * Corner HUD minimap (pure C++). Renders the dungeon generator's cell grid to a transient texture:
 * walkable cells reveal as the player explores (fog of war), the boss room tints red, and a bright
 * marker tracks the player. Regenerating the dungeon (grid size change) rebuilds the map + fog.
 */
UCLASS()
class DUNGEONCRAWLER_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Reveals the entire map (dev map-reveal); forces a repaint on the next tick. */
	void RevealEntireMap();

private:
	/** Locates the dungeon generator and (re)allocates the texture/fog buffers for its grid. */
	bool EnsureMap();
	/** Reveals walkable cells within RevealRadius of (CenterX,CenterY). */
	void RevealAround(int32 CenterX, int32 CenterY);
	/** Repaints the pixel buffer from cell types + fog + player marker, then pushes it to the texture. */
	void Repaint(int32 PlayerX, int32 PlayerY);

	UPROPERTY() TObjectPtr<UImage> MapImage;
	UPROPERTY() TObjectPtr<UTexture2D> MapTexture;
	UPROPERTY() TWeakObjectPtr<ADungeonGenerator> Generator;

	TArray<FColor> Pixels;   // GridW * GridH, BGRA
	TArray<bool> Revealed;   // fog-of-war mask, one per cell
	int32 GridW = 0;
	int32 GridH = 0;
	int32 LastPlayerX = -1;
	int32 LastPlayerY = -1;

	static constexpr int32 RevealRadius = 5; // cells revealed around the player
	static constexpr float DisplaySize = 220.f;
};
