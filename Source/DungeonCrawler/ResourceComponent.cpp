#include "ResourceComponent.h"
#include "Engine/World.h"

UResourceComponent::UResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UResourceComponent::BeginPlay()
{
	Super::BeginPlay();
	Current = MaxValue;
}

void UResourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bRegenPaused || RegenPerSecond <= 0.f || Current >= MaxValue)
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastSpendTime < RegenDelay)
	{
		return; // still in the post-spend cooldown
	}

	Current = FMath::Min(MaxValue, Current + RegenPerSecond * DeltaTime);
}

bool UResourceComponent::Spend(float Amount)
{
	if (Amount <= 0.f)
	{
		return true;
	}
	if (Current < Amount)
	{
		return false;
	}
	Current -= Amount;
	LastSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	return true;
}

void UResourceComponent::Drain(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	Current = FMath::Max(0.f, Current - Amount);
	LastSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void UResourceComponent::Restore(float Amount)
{
	if (Amount > 0.f)
	{
		Current = FMath::Min(MaxValue, Current + Amount);
	}
}

void UResourceComponent::SetMax(float NewMax, bool bRefill)
{
	const float OldPercent = GetPercent();
	MaxValue = FMath::Max(1.f, NewMax);
	Current = bRefill ? MaxValue : FMath::Clamp(OldPercent * MaxValue, 0.f, MaxValue);
}
