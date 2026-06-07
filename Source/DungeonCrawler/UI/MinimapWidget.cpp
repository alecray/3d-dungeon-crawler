#include "MinimapWidget.h"
#include "DungeonGenerator.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TextureResource.h"

// Palette (BGRA stored as FColor R/G/B mapped to PF_B8G8R8A8 below via FColor's natural layout).
static const FColor C_Hidden(10, 10, 14, 235);     // unexplored (dim, near-opaque)
static const FColor C_Floor(150, 150, 165, 255);   // revealed walkable cell
static const FColor C_BossFloor(205, 75, 75, 255); // revealed boss-room cell
static const FColor C_Player(90, 205, 255, 255);   // player marker

bool UMinimapWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		return true; // already built
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Dark framed backdrop in the top-right corner.
	const float Pad = 6.f;
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
	Frame->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f));
	Frame->SetPadding(FMargin(Pad));
	if (UCanvasPanelSlot* FS = Root->AddChildToCanvas(Frame))
	{
		FS->SetAnchors(FAnchors(1.f, 0.f)); // top-right
		FS->SetAlignment(FVector2D(1.f, 0.f));
		FS->SetPosition(FVector2D(-16.f, 16.f));
		FS->SetSize(FVector2D(DisplaySize + 2.f * Pad, DisplaySize + 2.f * Pad));
	}

	MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MapImage"));
	Frame->SetContent(MapImage);

	return true;
}

bool UMinimapWidget::EnsureMap()
{
	if (!Generator.IsValid())
	{
		Generator = Cast<ADungeonGenerator>(
			UGameplayStatics::GetActorOfClass(this, ADungeonGenerator::StaticClass()));
	}
	ADungeonGenerator* Gen = Generator.Get();
	if (!Gen)
	{
		return false;
	}

	const int32 W = Gen->GetGridWidth();
	const int32 H = Gen->GetGridHeight();
	if (W <= 0 || H <= 0)
	{
		return false;
	}

	// (Re)build the texture + fog when the grid size changes (first run or a regenerate).
	if (W != GridW || H != GridH || !MapTexture)
	{
		GridW = W;
		GridH = H;
		Pixels.Init(C_Hidden, GridW * GridH);
		Revealed.Init(false, GridW * GridH);
		LastPlayerX = LastPlayerY = -1;

		MapTexture = UTexture2D::CreateTransient(GridW, GridH, PF_B8G8R8A8);
		MapTexture->Filter = TF_Nearest;       // crisp cells, no blur when scaled up
		MapTexture->AddressX = TA_Clamp;
		MapTexture->AddressY = TA_Clamp;
		MapTexture->SRGB = true;
		MapTexture->UpdateResource();

		if (MapImage)
		{
			MapImage->SetBrushFromTexture(MapTexture, /*bMatchSize=*/false);
			MapImage->SetDesiredSizeOverride(FVector2D(DisplaySize, DisplaySize));
		}
	}
	return true;
}

void UMinimapWidget::RevealEntireMap()
{
	for (bool& Cell : Revealed)
	{
		Cell = true;
	}
	LastPlayerX = -1; // force a repaint on the next tick
	LastPlayerY = -1;
}

void UMinimapWidget::RevealAround(int32 CenterX, int32 CenterY)
{
	ADungeonGenerator* Gen = Generator.Get();
	if (!Gen)
	{
		return;
	}
	for (int32 dy = -RevealRadius; dy <= RevealRadius; ++dy)
	{
		for (int32 dx = -RevealRadius; dx <= RevealRadius; ++dx)
		{
			if (dx * dx + dy * dy > RevealRadius * RevealRadius)
			{
				continue; // circular reveal
			}
			const int32 X = CenterX + dx;
			const int32 Y = CenterY + dy;
			if (X < 0 || Y < 0 || X >= GridW || Y >= GridH)
			{
				continue;
			}
			if (Gen->IsFloorCell(X, Y))
			{
				Revealed[Y * GridW + X] = true;
			}
		}
	}
}

void UMinimapWidget::Repaint(int32 PlayerX, int32 PlayerY)
{
	ADungeonGenerator* Gen = Generator.Get();
	if (!Gen || !MapTexture || Pixels.Num() != GridW * GridH)
	{
		return;
	}

	for (int32 Y = 0; Y < GridH; ++Y)
	{
		for (int32 X = 0; X < GridW; ++X)
		{
			const int32 i = Y * GridW + X;
			if (!Revealed[i])
			{
				Pixels[i] = C_Hidden;
			}
			else
			{
				Pixels[i] = Gen->IsBossRoomCell(X, Y) ? C_BossFloor : C_Floor;
			}
		}
	}

	// Player marker on top (only meaningful while on the grid).
	if (PlayerX >= 0 && PlayerY >= 0 && PlayerX < GridW && PlayerY < GridH)
	{
		Pixels[PlayerY * GridW + PlayerX] = C_Player;
	}

	// UpdateTextureRegions keeps the region + data pointers for an async render-thread copy, so both
	// must outlive this scope: heap-allocate them and free in the cleanup callback (also avoids a race
	// with the next tick rewriting Pixels).
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, GridW, GridH);
	const int32 DataBytes = Pixels.Num() * sizeof(FColor);
	uint8* DataCopy = new uint8[DataBytes];
	FMemory::Memcpy(DataCopy, Pixels.GetData(), DataBytes);

	MapTexture->UpdateTextureRegions(0, 1, Region, GridW * sizeof(FColor), sizeof(FColor), DataCopy,
		[](uint8* Src, const FUpdateTextureRegion2D* Regions)
		{
			delete[] Src;
			delete Regions;
		});
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!EnsureMap())
	{
		return;
	}

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn)
	{
		return;
	}

	int32 PX = -1, PY = -1;
	Generator->WorldToCell(Pawn->GetActorLocation(), PX, PY);

	// Only repaint when the player crosses into a new cell (cheap; the marker only moves per-cell).
	if (PX == LastPlayerX && PY == LastPlayerY)
	{
		return;
	}
	LastPlayerX = PX;
	LastPlayerY = PY;

	RevealAround(PX, PY);
	Repaint(PX, PY);
}
