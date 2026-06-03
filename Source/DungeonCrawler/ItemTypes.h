#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

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
	Crossbow  // ranged
};

/** Static definition of an item kind (no art — a rarity color stands in for the icon for now). */
USTRUCT(BlueprintType)
struct FItemDef
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() FString DisplayName;
	UPROPERTY() EItemType Type = EItemType::Misc;
	UPROPERTY() EItemRarity Rarity = EItemRarity::Common;
	UPROPERTY() int32 MaxStack = 1;
	UPROPERTY() int32 Value = 1; // gold value (used by the shop later)
	UPROPERTY() EEquipKind EquipKind = EEquipKind::None; // equippable weapon kind, if any

	// Mesh used to render this item's UI icon (one of these; leave empty to use the rarity color).
	UPROPERTY() FString IconStaticMeshPath;
	UPROPERTY() FString IconSkeletalMeshPath;

	// Optional per-item texture swap on the icon mesh's material: a shared material with a texture
	// parameter (e.g. potions reuse SK_Potion/MI_Potion, recolored to health/mana/stamina). Both must
	// be set to take effect; applied via a Dynamic Material Instance when rendering the icon.
	UPROPERTY() FString IconMaterialParam; // texture parameter name on the material, e.g. "BaseColorTex"
	UPROPERTY() FString IconTexturePath;   // texture to bind to that parameter
};

/** One stack in an inventory: an item id + count. Empty when Id is None / Count <= 0. */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY() FName ItemId;
	UPROPERTY() int32 Count = 0;

	bool IsEmpty() const { return ItemId.IsNone() || Count <= 0; }
	void Clear() { ItemId = NAME_None; Count = 0; }
};

/** Placeholder-icon color for a rarity (until real icons exist). */
DUNGEONCRAWLER_API FLinearColor RarityColor(EItemRarity Rarity);

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
