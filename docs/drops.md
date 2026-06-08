# Item Drop & Acquisition Table

All items available in the game and every source from which they can be obtained.

> Generated from code on 2026-06-07; may drift — re-verify against `Source/DungeonCrawler/Items/ItemTypes.cpp`, `LootChest.cpp`, `BossArena.cpp`, `FishingHole.cpp`, `ShopNPC.cpp`, and `FirstPersonCharacter.cpp`.

---

## Master Table

| Display Name | ID | Type | Rarity | Value | Shop | Chest | Boss | Fishing | Starting |
|---|---|---|---|---|---|---|---|---|---|
| Health Potion | `HealthPotion` | Consumable | Common | 15g | ✓ | ✓ | ✓ | | |
| Mana Potion | `ManaPotion` | Consumable | Common | 15g | ✓ | ✓ | ✓ | | |
| Stamina Potion | `StaminaPotion` | Consumable | Common | 12g | ✓ | ✓ | ✓ | | |
| Old Bone | `Bone` | Material | Common | 2g | | ✓ | ✓ | | |
| Iron Shard | `IronShard` | Material | Common | 5g | | ✓ | ✓ | | |
| Sword | `Sword` | Weapon | Common | 40g | ✓ | ✓ | ✓ | | ✓ (hotbar slot 1) |
| Apprentice Wand | `Wand` | Weapon | Common | 45g | ✓ | ✓ | ✓ | | |
| Leather Gloves | `LeatherGloves` | Armor | Common | 30g | | ✓ | ✓ | | |
| Leather Belt | `LeatherBelt` | Armor | Common | 30g | | ✓ | ✓ | | |
| Leather Leggings | `LeatherLegs` | Armor | Common | 35g | | ✓ | ✓ | | |
| Leather Boots | `LeatherBoots` | Armor | Common | 30g | | ✓ | ✓ | | |
| Fishing Rod | `FishingRod` | Misc | Common | 35g | ✓ | ✓ | ✓ | | |
| Crossbow | `Crossbow` | Weapon | Uncommon | 80g | ✓ | ✓ | ✓ | | ✓ (inventory slot 2) |
| Iron Sword | `IronSword` | Weapon | Uncommon | 60g | ✓ | ✓ | ✓ | | |
| Oak Staff | `Staff` | Weapon | Uncommon | 90g | | ✓ | ✓ | | |
| Leather Armor | `LeatherArmor` | Armor | Uncommon | 55g | ✓ | ✓ | ✓ | | |
| Iron Helm | `IronHelm` | Armor | Uncommon | 50g | | ✓ | ✓ | | |
| Iron Ring | `IronRing` | Treasure | Uncommon | 45g | | ✓ | ✓ | | |
| Ruby Gem | `RubyGem` | Treasure | Rare | 120g | | ✓ | ✓ | | |
| Gold Amulet | `GoldAmulet` | Treasure | Rare | 90g | | ✓ | ✓ | | |
| Ruby Ring | `RubyRing` | Treasure | Rare | 80g | | ✓ | ✓ | | |
| Death Scroll | `DeathScroll` | Consumable | Rare | 200g | ✓ | ✓ | ✓ | | |
| Runed Blade | `RunedBlade` | Weapon | Epic | 400g | | ✓ | ✓ | | |
| Ancient Relic | `AncientRelic` | Treasure | Legendary | 1000g | | ✓ | ✓ | | |
| Minnow | `Minnow` | Material | Common | 3g | | | | ✓ | |
| Sardine | `Sardine` | Material | Common | 4g | | | | ✓ | |
| Old Boot | `OldBoot` | Material | Common | 1g | | | | ✓ | |
| Trout | `Trout` | Material | Uncommon | 10g | | | | ✓ | |
| Bass | `Bass` | Material | Uncommon | 14g | | | | ✓ | |
| Pike | `Pike` | Material | Rare | 30g | | | | ✓ | |
| Golden Carp | `GoldenCarp` | Treasure | Epic | 120g | | | | ✓ | |

---

## By Source

### Shop (`ShopNPC`)

The town shopkeeper stocks a fixed default inventory (overridable per-instance via the `Wares` property):

| Item | Rarity | Price |
|---|---|---|
| Health Potion | Common | 15g |
| Mana Potion | Common | 15g |
| Stamina Potion | Common | 12g |
| Sword | Common | 40g |
| Apprentice Wand | Common | 45g |
| Crossbow | Uncommon | 80g |
| Iron Sword | Uncommon | 60g |
| Leather Armor | Uncommon | 55g |
| Fishing Rod | Common | 35g |
| Death Scroll | Rare | 200g |

Note: Oak Staff (`Staff`) is **not sold** — it is chest/boss only.

---

### Loot Chests (`LootChest`)

- Drops **2–5 items** per chest, rolled on first open (result is fixed after that).
- Each slot is rolled independently via `RollRandomItem()` using rarity weights:

| Rarity | Weight | Approx. chance per slot |
|---|---|---|
| Common | 50 | ~58.8% |
| Uncommon | 22 | ~25.9% |
| Rare | 9 | ~10.6% |
| Epic | 3 | ~3.5% |
| Legendary | 1 | ~1.2% |

- Stackable items (potions, materials) drop 1–3 per slot; non-stackable items drop exactly 1.
- The chest emits a rarity-colored sparkle on open; Rare and above get a double burst.
- **Fishing-only items are excluded** from this pool (see Notes).

---

### Boss (`BossArena`)

- Drops **3–5 items** scattered around the kill site on boss defeat.
- Uses a **best-of-2 rolls** mechanic: two items are rolled via `RollRandomItem()` and the one with the higher rarity is kept. This skews drops meaningfully toward the upper end of the rarity table compared to chest loot.
- The same rarity weights apply (Common 50 / Uncommon 22 / Rare 9 / Epic 3 / Legendary 1), but the best-of-2 filter roughly halves the chance of ending up with a Common and doubles the effective rate of Rares and above.
- Stackable items drop 1–3 per slot.
- **Fishing-only items are excluded** from this pool (see Notes).

---

### Regular Enemies (`MonsterCharacter`)

Regular enemies drop **nothing**. On death they award XP only (via `StatsComponent::AddXP`). No item pickup is spawned.

---

### Fishing (`FishingHole`)

Requires a **Fishing Rod** in inventory. Cast a line at the town fishing hole; if a bite triggers and the player reels in within the window, the catch is resolved via `RollFish()`:

| Catch | ID | Rarity | Sell | Weight | Chance |
|---|---|---|---|---|---|
| Old Boot | `OldBoot` | Common | 1g | 8 | 8% |
| Minnow | `Minnow` | Common | 3g | 25 | 25% |
| Sardine | `Sardine` | Common | 4g | 23 | 23% |
| Trout | `Trout` | Uncommon | 10g | 18 | 18% |
| Bass | `Bass` | Uncommon | 14g | 14 | 14% |
| Pike | `Pike` | Rare | 30g | 9 | 9% |
| Golden Carp | `GoldenCarp` | Epic | 120g | 3 | 3% |

Total weight = 100. The Old Boot is junk and does not count toward the `FishCaught` stat.

Fish are the **only** source for these seven items — they are excluded from chest and boss loot rolls.

---

### Starting Loadout (fresh save)

On a brand-new profile `FirstPersonCharacter::BeginPlay` gives the player:

| Item | Location |
|---|---|
| Sword | Hotbar slot 1, auto-equipped |
| Crossbow | Inventory slot 2 (not hotbar) |

No gold, no potions, no armor — everything else must be bought, found in chests, or fished.

---

## Notes

- **Affixes scaffolded, not yet applied.** `AffixDatabase` defines six named modifiers (Flaming, of Frost, of the Inferno, of the Storm, of Venom, Savage) with a roller (`RollAffix`) and name/bonus composers (`ComposeItemName`, `ComposeItemBonuses`). The wiring onto actual drops is not yet complete — no dropped item currently receives an affix in-game.
- **Oak Staff shop gap.** `Staff` is intentionally absent from the shop's default stock; it is obtainable only from chests and the boss.
- **Fishing Rod in the loot pool.** The Fishing Rod (`FishingRod`) shares Common rarity and is present in the `RollRandomItem` pool, so it can appear as chest or boss loot, but buying it from the shop is the reliable path.
- **Death Scroll mechanic.** If the player dies while carrying a Death Scroll, it crumbles and restores half health, mana, and stamina instead of triggering a full death. Can stack (up to 5).
- **Equipment stat bonuses (while equipped):** Iron Helm +20 max HP; Leather Armor +35 max HP; Leather Gloves +15 max stamina; Leather Belt +15 max HP; Leather Leggings +20 max HP; Leather Boots +15 max stamina; Gold Amulet +25 max mana; Ruby Ring +6% spell damage; Iron Ring +6% melee damage.
