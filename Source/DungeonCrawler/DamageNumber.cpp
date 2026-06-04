#include "DamageNumber.h"

#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

ADamageNumber::ADamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	SetRootComponent(Text);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(48.f);
	Text->SetTextRenderColor(FColor::White);
	Text->SetText(FText::GetEmpty());
}

void ADamageNumber::Init(float Amount, bool bWeakPoint)
{
	const int32 Rounded = FMath::Max(1, FMath::RoundToInt(Amount));
	if (Text)
	{
		Text->SetText(FText::AsNumber(Rounded));
		// Big, bright, unlit so it pops through the dark/fog at combat range.
		Text->SetTextRenderColor(bWeakPoint ? FColor(255, 130, 0) : FColor(255, 250, 200));
		Text->SetWorldSize(bWeakPoint ? 240.f : 165.f);
	}

	// Face the camera immediately so it's never edge-on / mirrored on the first frame.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		SetActorRotation(FRotator(0.f, Cam->GetCameraRotation().Yaw + 180.f, 0.f));
	}

	// Drift up in a small arc with a little sideways scatter so stacked hits don't perfectly overlap.
	Velocity = FVector(FMath::FRandRange(-45.f, 45.f), FMath::FRandRange(-45.f, 45.f), 230.f);
	MaxLife = 1.5f; // lingers longer so it's easy to read
}

void ADamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Life += DeltaSeconds;
	const float A = (MaxLife > 0.f) ? (Life / MaxLife) : 1.f;

	AddActorWorldOffset(Velocity * DeltaSeconds);
	Velocity.Z -= 240.f * DeltaSeconds; // gentle deceleration/arc

	// Yaw-billboard toward the camera so the text always reads flat to the player.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		SetActorRotation(FRotator(0.f, Cam->GetCameraRotation().Yaw + 180.f, 0.f));
	}

	// Punchy pop on spawn, hold, then shrink out over the final third.
	if (Text)
	{
		float S = 1.f;
		if (A < 0.12f)        { S = FMath::Lerp(1.7f, 1.f, A / 0.12f); } // overshoot then settle
		else if (A > 0.66f)   { S = FMath::GetMappedRangeValueClamped(FVector2D(0.66f, 1.f), FVector2D(1.f, 0.f), A); }
		Text->SetWorldScale3D(FVector(S));
	}

	if (Life >= MaxLife)
	{
		Destroy();
	}
}
