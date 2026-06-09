#pragma once

#include "CoreMinimal.h"
#include "StatsComponent.h" // FCoreAttributes

/** Starting archetype chosen at the main menu. Plain enum (not exposed to Blueprint). */
enum class ECharacterClass : uint8
{
	Warrior, // melee / sword — high STR + VIT
	Ranger,  // ranged / crossbow — high DEX
	Mage     // spells / wand — high INT
};

/** The starting attribute spread + weapon for a class. */
struct FClassLoadout
{
	FCoreAttributes Attributes;
	FName WeaponId;
	FString Name;
};

/** Returns the starting loadout for a class (defaults to Warrior for unknown values). */
DUNGEONCRAWLER_API FClassLoadout GetClassLoadout(ECharacterClass Class);
