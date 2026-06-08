#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UTexture;

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Consumable,
	Weapon,
	Armor,
	Material,
	Treasure,
	Misc
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

/** What equipping this item does (drives the held weapon + combat style). None = not equippable. */
UENUM(BlueprintType)
enum class EEquipKind : uint8
{
	None,
	Sword,    // melee
	Crossbow, // ranged
	Staff     // mage (spell projectiles, costs mana)
};

/** Paperdoll equipment slot an item fits into (armor/accessories). None = not equipment. */
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
	None,
	Head,
	Amulet,
	Body,
	Gloves,
	Belt,
	Legs,
	Feet,
	Ring
};

/** Damage element — physical by default; affixes can add elemental damage ("of the Inferno" = fire). */
UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Physical,
	Fire,
	Frost,
	Lightning,
	Poison,
	Arcane
};

/** Stat bonuses an equipped item grants (added to UStatsComponent's equipment bonuses). */
USTRUCT(BlueprintType)
struct FItemBonuses
{
	GENERATED_BODY()

	UPROPERTY() float MaxHealth = 0.f;
	UPROPERTY() float MaxMana = 0.f;
	UPROPERTY() float MaxStamina = 0.f;
	UPROPERTY() float MeleeMult = 0.f;
	UPROPERTY() float RangedMult = 0.f;
	UPROPERTY() float SpellMult = 0.f;

	// Elemental damage rider (e.g. an affix granting "+X fire damage"). FlatBonusDamage of the given
	// element is added on hit; 0 = none. Combat application is wired up later — see FItemAffix.
	UPROPERTY() float FlatBonusDamage = 0.f;
	UPROPERTY() EDamageType BonusDamageType = EDamageType::Physical;

	/** Sums another bonus block into this one (used to fold rolled affixes onto a base item). */
	void Add(const FItemBonuses& O)
	{
		MaxHealth += O.MaxHealth; MaxMana += O.MaxMana; MaxStamina += O.MaxStamina;
		MeleeMult += O.MeleeMult; RangedMult += O.RangedMult; SpellMult += O.SpellMult;
		// Elemental riders don't sum cleanly across different elements; take the larger contributor.
		if (O.FlatBonusDamage > FlatBonusDamage) { FlatBonusDamage = O.FlatBonusDamage; BonusDamageType = O.BonusDamageType; }
	}
};

/**
 * A rolled item affix — a named modifier that contributes bonuses and a name fragment, so an item can
 * be titled from what it does (e.g. the "Inferno" affix → "Sword of the Inferno", +fire damage).
 *
 * SCAFFOLDING: the data model + name composition exist now; rolling affixes onto drops, persisting them
 * per inventory instance, and applying their bonuses in combat are wired up later (see TODO).
 */
USTRUCT(BlueprintType)
struct FItemAffix
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() FString NameFragment;            // e.g. "Flaming" (prefix) or "of the Inferno" (suffix)
	UPROPERTY() bool bSuffix = true;             // suffix ("Sword of the Inferno") vs prefix ("Flaming Sword")
	UPROPERTY() EItemRarity MinRarity = EItemRarity::Uncommon; // doesn't roll below this item rarity
	UPROPERTY() FItemBonuses Bonuses;            // what it grants
};

/** Static definition of an item kind (no art — a rarity color stands in for the icon for now). */
USTRUCT(BlueprintType)
struct FItemDef
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() FString DisplayName;
	UPROPERTY() FString Description; // flavor / usage text shown in tooltips
	UPROPERTY() EItemType Type = EItemType::Misc;
	UPROPERTY() EItemRarity Rarity = EItemRarity::Common;
	UPROPERTY() int32 MaxStack = 1;
	UPROPERTY() int32 Value = 1; // gold value (used by the shop later)
	UPROPERTY() EEquipKind EquipKind = EEquipKind::None; // equippable weapon kind, if any
	UPROPERTY() EEquipSlot EquipSlot = EEquipSlot::None; // paperdoll slot (armor/accessory), if any
	UPROPERTY() FItemBonuses Bonuses;                    // stat bonuses applied while equipped

	// Mesh used to render this item's UI icon (one of these; leave unset to use the rarity color).
	// Soft refs so renamed/moved assets are caught by reference fixup and cook validation, not silently dropped.
	UPROPERTY() TSoftObjectPtr<UStaticMesh> IconStaticMeshPath;
	UPROPERTY() TSoftObjectPtr<USkeletalMesh> IconSkeletalMeshPath;

	// Optional per-item texture swap on the icon mesh's material: a shared material with a texture
	// parameter (e.g. potions reuse SK_Potion/MI_Potion, recolored to health/mana/stamina). Both must
	// be set to take effect; applied via a Dynamic Material Instance when rendering the icon.
	UPROPERTY() FString IconMaterialParam;             // texture parameter name on the material, e.g. "BaseColorTex" (a name, not an asset)
	UPROPERTY() TSoftObjectPtr<UTexture> IconTexturePath; // texture to bind to that parameter

	/** Only obtainable by fishing; excluded from the random chest/boss loot roll (RollRandomItem skips these). */
	UPROPERTY() bool bFishingOnly = false;
};

/** One stack in an inventory: an item id + count. Empty when Id is None / Count <= 0. */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY() FName ItemId;
	UPROPERTY() int32 Count = 0;

	// Rolled affixes carried by THIS instance (empty for plain items). Drives the generated display name
	// + bonuses. Stored per-slot so two "swords" can differ; only items with no affixes stack together.
	// SCAFFOLDING: populated/applied later — present now so saves and stacking already account for it.
	UPROPERTY() TArray<FName> Affixes;

	bool IsEmpty() const { return ItemId.IsNone() || Count <= 0; }
	void Clear() { ItemId = NAME_None; Count = 0; Affixes.Reset(); }
};

/** Placeholder-icon color for a rarity (until real icons exist). */
DUNGEONCRAWLER_API FLinearColor RarityColor(EItemRarity Rarity);

/** Display names for rarity / type (used in tooltips). */
DUNGEONCRAWLER_API FString RarityName(EItemRarity Rarity);
DUNGEONCRAWLER_API FString ItemTypeName(EItemType Type);
DUNGEONCRAWLER_API FString DamageTypeName(EDamageType Type);

/**
 * Builds an item's display title from its base name + rolled affixes, e.g. base "Sword" + the "Inferno"
 * suffix affix → "Sword of the Inferno"; a prefix affix → "Flaming Sword". With no affixes it just
 * returns the base display name. The single highest-priority prefix/suffix is used (one of each).
 */
DUNGEONCRAWLER_API FString ComposeItemName(const FString& BaseName, const TArray<FName>& AffixIds);

/** The total bonuses an instance grants = base item bonuses folded with its rolled affixes' bonuses. */
DUNGEONCRAWLER_API FItemBonuses ComposeItemBonuses(const FItemBonuses& BaseBonuses, const TArray<FName>& AffixIds);

/** Code-defined affix registry (mirrors ItemDatabase). Empty/None-safe. */
namespace AffixDatabase
{
	DUNGEONCRAWLER_API const TArray<FItemAffix>& All();
	DUNGEONCRAWLER_API const FItemAffix* Find(FName Id);
	/** Picks a random affix id eligible for the given item rarity (None if rarity is too low / none fit). */
	DUNGEONCRAWLER_API FName RollAffix(struct FRandomStream& Rng, EItemRarity ItemRarity);
}

/** Lowercase label for an equipment slot (e.g. "head", "ammy"), used as the empty-slot placeholder. */
DUNGEONCRAWLER_API FString EquipSlotLabel(EEquipSlot Slot);

/**
 * Code-defined item registry (no DataTable asset). Look up definitions by id, enumerate all items,
 * and roll loot. Defined in C++ so items stay diff-friendly; swap to a DataTable later if desired.
 */
namespace ItemDatabase
{
	DUNGEONCRAWLER_API const FItemDef& Get(FName Id);
	DUNGEONCRAWLER_API bool Contains(FName Id);
	DUNGEONCRAWLER_API const TArray<FItemDef>& All();

	/** Picks a random item id weighted toward common rarities (for chest loot). */
	DUNGEONCRAWLER_API FName RollRandomItem(struct FRandomStream& Rng);
}
