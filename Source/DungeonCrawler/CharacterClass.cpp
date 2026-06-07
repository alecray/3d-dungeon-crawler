#include "CharacterClass.h"

// Starting attribute spreads. Each class invests its baseline points into the attribute its weapon
// scales with (DEX->ranged, INT->spell, STR->melee — see UStatsComponent's damage mults) plus enough
// VIT to survive, and dumps its off-stat. The four numbers sum to roughly the same total per class so
// no archetype starts strictly ahead. These are the floor; level-up attribute points layer on top.
FClassLoadout GetClassLoadout(ECharacterClass Class)
{
	FClassLoadout L;
	switch (Class)
	{
	case ECharacterClass::Ranger: // crossbow — DEX-heavy glass cannon, modest VIT
		L.Attributes.Strength = 5;
		L.Attributes.Intelligence = 4;
		L.Attributes.Dexterity = 9;
		L.Attributes.Vitality = 6;
		L.WeaponId = FName(TEXT("Crossbow"));
		L.Name = TEXT("Ranger");
		break;

	case ECharacterClass::Mage: // wand — INT-heavy caster, frailest (lowest STR + VIT)
		L.Attributes.Strength = 3;
		L.Attributes.Intelligence = 9;
		L.Attributes.Dexterity = 4;
		L.Attributes.Vitality = 5;
		L.WeaponId = FName(TEXT("Wand"));
		L.Name = TEXT("Mage");
		break;

	case ECharacterClass::Warrior: // sword — STR for damage, highest VIT (the durable bruiser); default
	default:
		L.Attributes.Strength = 9;
		L.Attributes.Intelligence = 3;
		L.Attributes.Dexterity = 4;
		L.Attributes.Vitality = 8;
		L.WeaponId = FName(TEXT("Sword"));
		L.Name = TEXT("Warrior");
		break;
	}
	return L;
}
