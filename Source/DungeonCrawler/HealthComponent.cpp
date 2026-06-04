#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

float UHealthComponent::ApplyDamage(float Amount)
{
	if (bDead || bInvulnerable || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Before = Health;
	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	const float Applied = Before - Health;

	if (Applied > 0.f)
	{
		OnDamaged.Broadcast(this, Applied);
	}

	if (Health <= 0.f && !bDead)
	{
		bDead = true;
		OnDepleted.Broadcast(this);
	}
	return Applied;
}

void UHealthComponent::Kill()
{
	if (bDead)
	{
		return;
	}
	Health = 0.f;
	bDead = true;
	OnDepleted.Broadcast(this);
}

void UHealthComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
}

void UHealthComponent::SetMaxHealth(float NewMax, bool bRefill)
{
	MaxHealth = FMath::Max(1.f, NewMax);
	if (bRefill)
	{
		Health = MaxHealth;
		bDead = false;
	}
	else
	{
		Health = FMath::Min(Health, MaxHealth);
	}
}
