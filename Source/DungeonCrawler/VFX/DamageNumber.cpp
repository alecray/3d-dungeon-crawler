#include "DamageNumber.h"

#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPath.h"

// Ease-out-back: overshoots past the target then settles — the "cartoon thud" feel.
// x = normalized time [0..1], returns value that goes above 1.0 briefly then lands at 1.0.
static float EaseOutBack(float x)
{
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.f;
	return 1.f + c3 * FMath::Pow(x - 1.f, 3.f) + c1 * FMath::Pow(x - 1.f, 2.f);
}

ADamageNumber::ADamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	// Outline layer — rendered first (behind), slightly larger, black.
	Outline = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Outline"));
	SetRootComponent(Outline);
	Outline->SetHorizontalAlignment(EHTA_Center);
	Outline->SetVerticalAlignment(EVRTA_TextCenter);
	Outline->SetWorldSize(52.f);
	Outline->SetTextRenderColor(FColor::Black);
	Outline->SetText(FText::GetEmpty());
	// Push outline 2cm behind the main text so it's always drawn under it.
	Outline->SetRelativeLocation(FVector(-2.f, 0.f, 0.f));

	// Main text on top of the outline.
	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	Text->SetupAttachment(Outline);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(48.f);
	Text->SetTextRenderColor(FColor::White);
	Text->SetText(FText::GetEmpty());

	// Load the translucent/unlit engine text material so vertex color = screen color
	// regardless of dungeon lighting (the default opaque material is lit — appears black in the dark).
	if (UMaterialInterface* UnlitMat = Cast<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/EngineMaterials/DefaultTextMaterialTranslucent.DefaultTextMaterialTranslucent")).TryLoad()))
	{
		Text->SetMaterial(0, UnlitMat);
		Outline->SetMaterial(0, UnlitMat);
	}
}

void ADamageNumber::Init(float Amount, bool bWeakPoint)
{
	const int32 Rounded = FMath::Max(1, FMath::RoundToInt(Amount));

	// Size scales with damage magnitude (sqrt curve: 5 dmg feels clearly smaller than 50,
	// but 500 vs 5000 doesn't produce absurd numbers). Crits get an extra size spike.
	const float MagSize = FMath::Clamp(80.f + 40.f * FMath::Sqrt(Amount / 10.f), 80.f, 280.f);
	const float MainSize  = bWeakPoint ? FMath::Min(MagSize * 1.4f, 310.f) : MagSize;
	const float OutlineSize = MainSize * 1.12f; // outline slightly larger to form the border

	if (Text)
	{
		Text->SetText(FText::AsNumber(Rounded));
		Text->SetTextRenderColor(bWeakPoint ? FColor(255, 90, 20) : FColor(255, 240, 160));
		Text->SetWorldSize(MainSize);
	}
	if (Outline)
	{
		Outline->SetText(FText::AsNumber(Rounded));
		Outline->SetWorldSize(OutlineSize);
	}

	// Face the camera immediately so it's never edge-on on the first frame.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		// Small random Z rotation so stacked numbers each look hand-placed, not templated.
		const float ZRot = Cam->GetCameraRotation().Yaw + 180.f + FMath::FRandRange(-10.f, 10.f);
		SetActorRotation(FRotator(0.f, ZRot, 0.f));
	}

	// Fast upward pop with lateral scatter. Crits fly a bit higher.
	Velocity = FVector(
		FMath::FRandRange(-35.f, 35.f),
		FMath::FRandRange(-35.f, 35.f),
		bWeakPoint ? 210.f : 170.f);

	MaxLife   = bWeakPoint ? 1.2f : 0.9f;
	bApexHeld    = false;
	ApexHoldLeft = 0.f;
}

void ADamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Life += DeltaSeconds;
	const float A = (MaxLife > 0.f) ? (Life / MaxLife) : 1.f;

	// --- Motion ---
	if (bApexHeld)
	{
		// Hold at the apex so the player has a moment to read the value.
		ApexHoldLeft -= DeltaSeconds;
		if (ApexHoldLeft <= 0.f)
		{
			bApexHeld = false;
			Velocity.Z = -30.f; // gentle descent after the hold
		}
	}
	else
	{
		AddActorWorldOffset(Velocity * DeltaSeconds);

		const float PrevZ = Velocity.Z;
		Velocity.Z -= 260.f * DeltaSeconds;

		// Detect the moment we pass the apex (Z velocity crosses zero) and start the hold.
		if (!bApexHeld && PrevZ > 0.f && Velocity.Z <= 0.f)
		{
			bApexHeld    = true;
			ApexHoldLeft = ApexHoldDuration;
			Velocity.Z   = 0.f;
		}
	}

	// --- Billboard to camera ---
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		// Preserve the random Z tilt from Init; only update yaw to track the camera.
		FRotator Rot = GetActorRotation();
		Rot.Yaw = Cam->GetCameraRotation().Yaw + 180.f;
		SetActorRotation(Rot);
	}

	// --- Scale animation ---
	// First 20% of life: ease-out-back punch (overshoots past 1.0 then settles) — the "cartoon thud."
	// Middle: hold at 1.0.
	// Final 30%: shrink + fade out.
	if (Text && Outline)
	{
		float S = 1.f;
		if (A < 0.20f)
		{
			// EaseOutBack on [0..1] mapped from the first 20% of life.
			S = FMath::Lerp(1.8f, 1.f, EaseOutBack(A / 0.20f));
			S = FMath::Max(S, 0.1f); // clamp against the brief undershoot
		}
		else if (A > 0.70f)
		{
			S = FMath::GetMappedRangeValueClamped(FVector2D(0.70f, 1.f), FVector2D(1.f, 0.f), A);
		}
		Outline->SetRelativeScale3D(FVector(S));
		Text->SetRelativeScale3D(FVector(S));
	}

	if (Life >= MaxLife)
	{
		Destroy();
	}
}
