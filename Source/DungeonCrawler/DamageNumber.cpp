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
		// Bright, unlit, big enough to read through the dungeon's fog at combat range.
		Text->SetTextRenderColor(bWeakPoint ? FColor(255, 140, 0) : FColor(255, 240, 120));
		Text->SetWorldSize(bWeakPoint ? 160.f : 110.f);
	}

	// Face the camera immediately so it's never edge-on / mirrored on the first frame.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		SetActorRotation(FRotator(0.f, Cam->GetCameraRotation().Yaw + 180.f, 0.f));
	}

	// Drift up in a small arc with a little sideways scatter so stacked hits don't perfectly overlap.
	Velocity = FVector(FMath::FRandRange(-45.f, 45.f), FMath::FRandRange(-45.f, 45.f), 200.f);
	MaxLife = 1.0f;
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

	// Shrink out over the final third.
	if (Text && A > 0.66f)
	{
		const float S = FMath::GetMappedRangeValueClamped(FVector2D(0.66f, 1.f), FVector2D(1.f, 0.f), A);
		Text->SetWorldScale3D(FVector(S));
	}

	if (Life >= MaxLife)
	{
		Destroy();
	}
}
