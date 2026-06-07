#include "ItemTypes.h"
#include "Math/RandomStream.h"

FLinearColor RarityColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Common:    return FLinearColor(0.55f, 0.55f, 0.55f); // gray
	case EItemRarity::Uncommon:  return FLinearColor(0.25f, 0.7f, 0.25f);  // green
	case EItemRarity::Rare:      return FLinearColor(0.2f, 0.45f, 0.95f);  // blue
	case EItemRarity::Epic:      return FLinearColor(0.65f, 0.3f, 0.9f);   // purple
	case EItemRarity::Legendary: return FLinearColor(0.95f, 0.6f, 0.15f);  // orange
	default:                     return FLinearColor::Gray;
	}
}

FString RarityName(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Common:    return TEXT("Common");
	case EItemRarity::Uncommon:  return TEXT("Uncommon");
	case EItemRarity::Rare:      return TEXT("Rare");
	case EItemRarity::Epic:      return TEXT("Epic");
	case EItemRarity::Legendary: return TEXT("Legendary");
	default:                     return TEXT("");
	}
}

FString EquipSlotLabel(EEquipSlot Slot)
{
	switch (Slot)
	{
	case EEquipSlot::Head:   return TEXT("head");
	case EEquipSlot::Amulet: return TEXT("ammy");
	case EEquipSlot::Body:   return TEXT("body");
	case EEquipSlot::Gloves: return TEXT("gloves");
	case EEquipSlot::Belt:   return TEXT("belt");
	case EEquipSlot::Legs:   return TEXT("legs");
	case EEquipSlot::Feet:   return TEXT("feet");
	case EEquipSlot::Ring:   return TEXT("ring");
	default:                 return TEXT("");
	}
}

FString ItemTypeName(EItemType Type)
{
	switch (Type)
	{
	case EItemType::Consumable: return TEXT("Consumable");
	case EItemType::Weapon:     return TEXT("Weapon");
	case EItemType::Armor:      return TEXT("Armor");
	case EItemType::Material:   return TEXT("Material");
	case EItemType::Treasure:   return TEXT("Treasure");
	default:                    return TEXT("Misc");
	}
}

FString DamageTypeName(EDamageType Type)
{
	switch (Type)
	{
	case EDamageType::Fire:      return TEXT("Fire");
	case EDamageType::Frost:     return TEXT("Frost");
	case EDamageType::Lightning: return TEXT("Lightning");
	case EDamageType::Poison:    return TEXT("Poison");
	case EDamageType::Arcane:    return TEXT("Arcane");
	default:                     return TEXT("Physical");
	}
}

// ---- Affixes (named item modifiers) --------------------------------------------------------------
// SCAFFOLDING: a small starter table so the naming pipeline is exercisable now. Expand later; the
// remaining wiring (rolling onto drops, applying bonuses in combat) is tracked in TODO.md.
namespace AffixDatabase
{
	static FItemAffix MakeAffix(const TCHAR* Id, const TCHAR* Fragment, bool bSuffix, EItemRarity MinRarity,
		EDamageType Dmg, float DmgAmount, float MeleeMult = 0.f)
	{
		FItemAffix A;
		A.Id = Id;
		A.NameFragment = Fragment;
		A.bSuffix = bSuffix;
		A.MinRarity = MinRarity;
		A.Bonuses.BonusDamageType = Dmg;
		A.Bonuses.FlatBonusDamage = DmgAmount;
		A.Bonuses.MeleeMult = MeleeMult;
		return A;
	}

	const TArray<FItemAffix>& All()
	{
		static const TArray<FItemAffix> Affixes = {
			MakeAffix(TEXT("inferno"),   TEXT("of the Inferno"), /*suffix*/ true,  EItemRarity::Uncommon, EDamageType::Fire,      12.f),
			MakeAffix(TEXT("frost"),     TEXT("of Frost"),       /*suffix*/ true,  EItemRarity::Uncommon, EDamageType::Frost,     9.f),
			MakeAffix(TEXT("storm"),     TEXT("of the Storm"),   /*suffix*/ true,  EItemRarity::Rare,     EDamageType::Lightning, 15.f),
			MakeAffix(TEXT("venom"),     TEXT("of Venom"),       /*suffix*/ true,  EItemRarity::Uncommon, EDamageType::Poison,    7.f),
			MakeAffix(TEXT("flaming"),   TEXT("Flaming"),        /*prefix*/ false, EItemRarity::Uncommon, EDamageType::Fire,      8.f),
			MakeAffix(TEXT("savage"),    TEXT("Savage"),         /*prefix*/ false, EItemRarity::Rare,     EDamageType::Physical,  0.f, /*melee*/ 0.15f),
		};
		return Affixes;
	}

	const FItemAffix* Find(FName Id)
	{
		if (Id.IsNone()) { return nullptr; }
		for (const FItemAffix& A : All()) { if (A.Id == Id) { return &A; } }
		return nullptr;
	}

	FName RollAffix(FRandomStream& Rng, EItemRarity ItemRarity)
	{
		TArray<FName> Eligible;
		for (const FItemAffix& A : All())
		{
			if ((uint8)ItemRarity >= (uint8)A.MinRarity) { Eligible.Add(A.Id); }
		}
		return Eligible.Num() > 0 ? Eligible[Rng.RandRange(0, Eligible.Num() - 1)] : NAME_None;
	}
}

FString ComposeItemName(const FString& BaseName, const TArray<FName>& AffixIds)
{
	// One prefix + one suffix at most; the first of each in the list wins.
	FString Prefix, Suffix;
	for (const FName& Id : AffixIds)
	{
		const FItemAffix* A = AffixDatabase::Find(Id);
		if (!A) { continue; }
		if (A->bSuffix) { if (Suffix.IsEmpty()) { Suffix = A->NameFragment; } }
		else            { if (Prefix.IsEmpty()) { Prefix = A->NameFragment; } }
	}

	FString Name = BaseName;
	if (!Prefix.IsEmpty()) { Name = Prefix + TEXT(" ") + Name; }
	if (!Suffix.IsEmpty()) { Name = Name + TEXT(" ") + Suffix; }
	return Name;
}

FItemBonuses ComposeItemBonuses(const FItemBonuses& BaseBonuses, const TArray<FName>& AffixIds)
{
	FItemBonuses Total = BaseBonuses;
	for (const FName& Id : AffixIds)
	{
		if (const FItemAffix* A = AffixDatabase::Find(Id)) { Total.Add(A->Bonuses); }
	}
	return Total;
}

// The master item catalog (cached once, queried by id). BuildItems() is intentionally multi-pass: it
// first creates every item with its core fields, then a series of helper lambdas patch in the optional
// data (equip slot + stat bonuses, tooltip text, icon meshes/textures) by id. Splitting it this way
// keeps the big readable item list uncluttered and lets the optional data live in grouped, scannable
// blocks rather than as long argument lists on every MakeItem call.
namespace ItemDatabase
{
	static TArray<FItemDef> BuildItems()
	{
		auto MakeItem = [](const TCHAR* Id, const TCHAR* Name, EItemType Type, EItemRarity Rarity, int32 MaxStack, int32 Value, EEquipKind Equip = EEquipKind::None)
		{
			FItemDef D;
			D.Id = FName(Id);
			D.DisplayName = Name;
			D.Type = Type;
			D.Rarity = Rarity;
			D.MaxStack = MaxStack;
			D.Value = Value;
			D.EquipKind = Equip;
			return D;
		};

		TArray<FItemDef> Items;
		Items.Add(MakeItem(TEXT("HealthPotion"), TEXT("Health Potion"), EItemType::Consumable, EItemRarity::Common, 10, 15));
		Items.Add(MakeItem(TEXT("ManaPotion"),   TEXT("Mana Potion"),   EItemType::Consumable, EItemRarity::Common, 10, 15));
		Items.Add(MakeItem(TEXT("StaminaPotion"),TEXT("Stamina Potion"),EItemType::Consumable, EItemRarity::Common, 10, 12));
		Items.Add(MakeItem(TEXT("Bone"),         TEXT("Old Bone"),      EItemType::Material,   EItemRarity::Common, 20, 2));
		Items.Add(MakeItem(TEXT("IronShard"),    TEXT("Iron Shard"),    EItemType::Material,   EItemRarity::Common, 20, 5));
		// Equippable weapons (action-bar / equip system).
		Items.Add(MakeItem(TEXT("Sword"),        TEXT("Sword"),         EItemType::Weapon,     EItemRarity::Common,   1, 40, EEquipKind::Sword));
		Items.Add(MakeItem(TEXT("Crossbow"),     TEXT("Crossbow"),      EItemType::Weapon,     EItemRarity::Uncommon, 1, 80, EEquipKind::Crossbow));
		Items.Add(MakeItem(TEXT("IronSword"),    TEXT("Iron Sword"),    EItemType::Weapon,     EItemRarity::Uncommon, 1, 60, EEquipKind::Sword));
		// Mage weapons — equipping one sets the Mage style (spell bolts cost mana).
		Items.Add(MakeItem(TEXT("Wand"),         TEXT("Apprentice Wand"),EItemType::Weapon,    EItemRarity::Common,   1, 45, EEquipKind::Staff));
		Items.Add(MakeItem(TEXT("Staff"),        TEXT("Oak Staff"),     EItemType::Weapon,     EItemRarity::Uncommon, 1, 90, EEquipKind::Staff));
		Items.Add(MakeItem(TEXT("LeatherArmor"), TEXT("Leather Armor"), EItemType::Armor,      EItemRarity::Uncommon, 1, 55));
		Items.Add(MakeItem(TEXT("RubyGem"),      TEXT("Ruby Gem"),      EItemType::Treasure,   EItemRarity::Rare,     5, 120));
		Items.Add(MakeItem(TEXT("RunedBlade"),   TEXT("Runed Blade"),   EItemType::Weapon,     EItemRarity::Epic,     1, 400, EEquipKind::Sword));
		Items.Add(MakeItem(TEXT("AncientRelic"), TEXT("Ancient Relic"), EItemType::Treasure,   EItemRarity::Legendary, 1, 1000));
		// Equippable armor & accessories (paperdoll slots).
		Items.Add(MakeItem(TEXT("IronHelm"),      TEXT("Iron Helm"),      EItemType::Armor, EItemRarity::Uncommon, 1, 50));
		Items.Add(MakeItem(TEXT("LeatherGloves"), TEXT("Leather Gloves"), EItemType::Armor, EItemRarity::Common,   1, 30));
		Items.Add(MakeItem(TEXT("LeatherBelt"),   TEXT("Leather Belt"),   EItemType::Armor, EItemRarity::Common,   1, 30));
		Items.Add(MakeItem(TEXT("LeatherLegs"),   TEXT("Leather Leggings"),EItemType::Armor,EItemRarity::Common,   1, 35));
		Items.Add(MakeItem(TEXT("LeatherBoots"),  TEXT("Leather Boots"),  EItemType::Armor, EItemRarity::Common,   1, 30));
		Items.Add(MakeItem(TEXT("GoldAmulet"),    TEXT("Gold Amulet"),    EItemType::Treasure, EItemRarity::Rare,  1, 90));
		Items.Add(MakeItem(TEXT("RubyRing"),      TEXT("Ruby Ring"),      EItemType::Treasure, EItemRarity::Rare,  1, 80));
		Items.Add(MakeItem(TEXT("IronRing"),      TEXT("Iron Ring"),      EItemType::Treasure, EItemRarity::Uncommon, 1, 45));

		// Fish (caught at the town fishing hole; stack + sell).
		Items.Add(MakeItem(TEXT("Minnow"),      TEXT("Minnow"),       EItemType::Material, EItemRarity::Common,    20, 3));
		Items.Add(MakeItem(TEXT("Sardine"),     TEXT("Sardine"),      EItemType::Material, EItemRarity::Common,    20, 4));
		Items.Add(MakeItem(TEXT("Trout"),       TEXT("Trout"),        EItemType::Material, EItemRarity::Uncommon,  20, 10));
		Items.Add(MakeItem(TEXT("Bass"),        TEXT("Bass"),         EItemType::Material, EItemRarity::Uncommon,  20, 14));
		Items.Add(MakeItem(TEXT("Pike"),        TEXT("Pike"),         EItemType::Material, EItemRarity::Rare,      20, 30));
		Items.Add(MakeItem(TEXT("GoldenCarp"),  TEXT("Golden Carp"),  EItemType::Treasure, EItemRarity::Epic,      5, 120));
		Items.Add(MakeItem(TEXT("OldBoot"),     TEXT("Old Boot"),     EItemType::Material, EItemRarity::Common,    20, 1)); // junk catch

		// Tools.
		Items.Add(MakeItem(TEXT("FishingRod"),  TEXT("Fishing Rod"),  EItemType::Misc,     EItemRarity::Common,     1, 35)); // required to fish

		// Equipment slot + stat bonuses (applied while equipped).
		auto SetEquip = [&Items](const TCHAR* Id, EEquipSlot Slot, const FItemBonuses& Bonuses)
		{
			const FName Key(Id);
			for (FItemDef& D : Items) { if (D.Id == Key) { D.EquipSlot = Slot; D.Bonuses = Bonuses; } }
		};
		auto Hp  = [](float V) { FItemBonuses B; B.MaxHealth = V;  return B; };
		auto Mp  = [](float V) { FItemBonuses B; B.MaxMana = V;    return B; };
		auto Sp  = [](float V) { FItemBonuses B; B.MaxStamina = V; return B; };
		auto Spl = [](float V) { FItemBonuses B; B.SpellMult = V;  return B; };
		auto Mel = [](float V) { FItemBonuses B; B.MeleeMult = V;  return B; };
		SetEquip(TEXT("IronHelm"),      EEquipSlot::Head,   Hp(20.f));
		SetEquip(TEXT("LeatherArmor"),  EEquipSlot::Body,   Hp(35.f));
		SetEquip(TEXT("LeatherGloves"), EEquipSlot::Gloves, Sp(15.f));
		SetEquip(TEXT("LeatherBelt"),   EEquipSlot::Belt,   Hp(15.f));
		SetEquip(TEXT("LeatherLegs"),   EEquipSlot::Legs,   Hp(20.f));
		SetEquip(TEXT("LeatherBoots"),  EEquipSlot::Feet,   Sp(15.f));
		SetEquip(TEXT("GoldAmulet"),    EEquipSlot::Amulet, Mp(25.f));
		SetEquip(TEXT("RubyRing"),      EEquipSlot::Ring,   Spl(0.06f));
		SetEquip(TEXT("IronRing"),      EEquipSlot::Ring,   Mel(0.06f));

		// Tooltip descriptions.
		auto SetDesc = [&Items](const TCHAR* Id, const TCHAR* Desc)
		{
			const FName Key(Id);
			for (FItemDef& D : Items) { if (D.Id == Key) { D.Description = Desc; } }
		};
		SetDesc(TEXT("HealthPotion"),  TEXT("Restores health when consumed."));
		SetDesc(TEXT("ManaPotion"),    TEXT("Restores mana when consumed."));
		SetDesc(TEXT("StaminaPotion"), TEXT("Restores stamina when consumed."));
		SetDesc(TEXT("Bone"),          TEXT("A weathered old bone. Worth little."));
		SetDesc(TEXT("IronShard"),     TEXT("A shard of raw iron. A crafting material."));
		SetDesc(TEXT("Sword"),         TEXT("A basic iron sword. Reliable in melee."));
		SetDesc(TEXT("Crossbow"),      TEXT("Fires bolts at range. Costs stamina."));
		SetDesc(TEXT("IronSword"),     TEXT("A sturdier blade than the common sword."));
		SetDesc(TEXT("LeatherArmor"),  TEXT("Light armor offering modest protection."));
		SetDesc(TEXT("RubyGem"),       TEXT("A precious gem, prized by merchants."));
		SetDesc(TEXT("RunedBlade"),    TEXT("A sword etched with faintly glowing runes."));
		SetDesc(TEXT("AncientRelic"),  TEXT("A mysterious relic of immense value."));
		SetDesc(TEXT("IronHelm"),      TEXT("An iron helm. +20 max health."));
		SetDesc(TEXT("LeatherGloves"), TEXT("Supple gloves. +15 max stamina."));
		SetDesc(TEXT("LeatherBelt"),   TEXT("A sturdy belt. +15 max health."));
		SetDesc(TEXT("LeatherLegs"),   TEXT("Leather leggings. +20 max health."));
		SetDesc(TEXT("LeatherBoots"),  TEXT("Worn boots. +15 max stamina."));
		SetDesc(TEXT("GoldAmulet"),    TEXT("A gilded amulet. +25 max mana."));
		SetDesc(TEXT("RubyRing"),      TEXT("A ruby ring. +6% spell damage."));
		SetDesc(TEXT("IronRing"),      TEXT("An iron band. +6% melee damage."));
		SetDesc(TEXT("FishingRod"),    TEXT("A simple fishing rod. Needed to cast a line at the fishing hole."));

		// Icon meshes (rendered into UI thumbnails). Weapons reuse their skeletal meshes; items
		// without a mesh fall back to the rarity color.
		auto SetSkelIcon = [&Items](const TCHAR* Id, const TCHAR* Path)
		{
			const FName Key(Id);
			for (FItemDef& D : Items) { if (D.Id == Key) { D.IconSkeletalMeshPath = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Path)); } }
		};
		SetSkelIcon(TEXT("Sword"),      TEXT("/Game/Weapons/Sword/SK_Sword.SK_Sword"));
		SetSkelIcon(TEXT("IronSword"),  TEXT("/Game/Weapons/Sword/SK_Sword.SK_Sword"));
		SetSkelIcon(TEXT("RunedBlade"), TEXT("/Game/Weapons/Sword/SK_Sword.SK_Sword"));
		SetSkelIcon(TEXT("Crossbow"),   TEXT("/Game/Weapons/Crossbow/SK_Crossbow.SK_Crossbow"));
		SetSkelIcon(TEXT("FishingRod"), TEXT("/Game/Tools/SK_Fishing_Rod.SK_Fishing_Rod"));

		// Potions share one mesh/material (SK_Potion + MI_Potion) and recolor via a texture parameter.
		auto SetPotionIcon = [&Items](const TCHAR* Id, const TCHAR* TexturePath)
		{
			const FName Key(Id);
			for (FItemDef& D : Items)
			{
				if (D.Id == Key)
				{
					D.IconStaticMeshPath = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Consumable/SM_Potion.SM_Potion")));
					D.IconMaterialParam = TEXT("BaseColorTex"); // texture param exposed on MI_Potion's parent material
					D.IconTexturePath = TSoftObjectPtr<UTexture>(FSoftObjectPath(TexturePath));
				}
			}
		};
		SetPotionIcon(TEXT("HealthPotion"), TEXT("/Game/Consumable/T_Health_BC.T_Health_BC"));
		SetPotionIcon(TEXT("ManaPotion"),    TEXT("/Game/Consumable/T_Mana_BC.T_Mana_BC"));
		SetPotionIcon(TEXT("StaminaPotion"), TEXT("/Game/Consumable/T_Stamina_BC.T_Stamina_BC"));

		return Items;
	}

	static const TArray<FItemDef>& Items()
	{
		static const TArray<FItemDef> Cached = BuildItems();
		return Cached;
	}

	static const TMap<FName, int32>& IndexById()
	{
		static const TMap<FName, int32> Map = []()
		{
			TMap<FName, int32> M;
			const TArray<FItemDef>& List = Items();
			for (int32 i = 0; i < List.Num(); ++i)
			{
				M.Add(List[i].Id, i);
			}
			return M;
		}();
		return Map;
	}

	const FItemDef& Get(FName Id)
	{
		if (const int32* Index = IndexById().Find(Id))
		{
			return Items()[*Index];
		}
		static const FItemDef Invalid;
		return Invalid;
	}

	bool Contains(FName Id)
	{
		return IndexById().Contains(Id);
	}

	const TArray<FItemDef>& All()
	{
		return Items();
	}

	FName RollRandomItem(FRandomStream& Rng)
	{
		// Weight by rarity: commons are far more likely than legendaries.
		const TArray<FItemDef>& List = Items();
		float Total = 0.f;
		TArray<float> Weights;
		Weights.Reserve(List.Num());
		for (const FItemDef& D : List)
		{
			float W = 1.f;
			switch (D.Rarity)
			{
			case EItemRarity::Common:    W = 50.f; break;
			case EItemRarity::Uncommon:  W = 22.f; break;
			case EItemRarity::Rare:      W = 9.f;  break;
			case EItemRarity::Epic:      W = 3.f;  break;
			case EItemRarity::Legendary: W = 1.f;  break;
			}
			Weights.Add(W);
			Total += W;
		}

		float Pick = Rng.FRandRange(0.f, Total);
		for (int32 i = 0; i < List.Num(); ++i)
		{
			Pick -= Weights[i];
			if (Pick <= 0.f)
			{
				return List[i].Id;
			}
		}
		return List.Num() > 0 ? List[0].Id : NAME_None;
	}
}
