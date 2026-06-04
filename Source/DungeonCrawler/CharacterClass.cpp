#include "CharacterClass.h"

FClassLoadout GetClassLoadout(ECharacterClass Class)
{
	FClassLoadout L;
	switch (Class)
	{
	case ECharacterClass::Ranger:
		L.Attributes.Strength = 5;
		L.Attributes.Intelligence = 4;
		L.Attributes.Dexterity = 9;
		L.Attributes.Vitality = 6;
		L.WeaponId = FName(TEXT("Crossbow"));
		L.Name = TEXT("Ranger");
		break;

	case ECharacterClass::Mage:
		L.Attributes.Strength = 3;
		L.Attributes.Intelligence = 9;
		L.Attributes.Dexterity = 4;
		L.Attributes.Vitality = 5;
		L.WeaponId = FName(TEXT("Wand"));
		L.Name = TEXT("Mage");
		break;

	case ECharacterClass::Warrior:
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
