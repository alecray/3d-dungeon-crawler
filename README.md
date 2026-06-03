# 3d-dungeon-crawler

First-person, procedurally generated dungeon crawler. **UE 5.7, pure C++** (no Blueprints), all
graybox so Blender assets can drop in later without code changes.

## Running

1. Build the editor target:
   ```
   "Z:\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" DungeonCrawlerEditor Win64 Development -project="%CD%\DungeonCrawler.uproject" -waitmutex
   ```
2. Open `DungeonCrawler.uproject` in Unreal Editor, then **Play**. The game mode builds the whole
   world at runtime — any empty level works; no map asset is needed.

**Controls:** WASD to move, mouse to look, Shift to sprint, Space to jump, LMB to attack (melee swing
/ ranged bolt / spell depending on the equipped weapon), **I** for inventory, **K** for the skill tree,
**1–8** to select the hotbar slot (swaps the held weapon), **E** to interact with chests / pickups.

## Core Feature Plan

Built so far:

- [x] First-person controller (WASD + mouse look, jump, sprint)
- [x] Procedurally generated dungeon (rooms + hallways, scattered props)
- [x] Custom floor/wall meshes + textured furniture (stool/table/crate); floor mesh reused as the
      ceiling (underside shown for separate texturing); walls alternate facing for variety
- [x] Stylized lighting (Lumen) + amber wall torches
- [x] Health & damage system (player + monsters)
- [x] Melee combat (skeletal sword + swing animation)
- [x] Enemy AI (chase/attack, hit-react) + 3-phase morphing boss
- [x] Player death → level restart
- [x] Attributes (Strength, Intelligence, Dexterity, Vitality) + level/XP progression (XP on kills,
      melee scales with Strength)
- [x] Health / Mana / Stamina bars (pure-C++ UMG HUD); stamina drives sprint
- [x] Save persistence (profile: attributes/level/XP/gold via a Game Instance + SaveGame)

Planned RPG systems (art-independent, in dependency order):

- [ ] Stats screen to spend attribute points
- [x] Inventory system with draggable items (Diablo-style)
- [x] Lootable chests
- [x] Collection log (rare items)
- [x] 3D item icons (item meshes rendered to cached textures; UI slots + world pickups share them,
      with per-item texture recolor — e.g. health/mana/stamina potions reuse one mesh)
- [x] Equipment & action bar (8 slots, equip weapons, drop items to world)
- [x] Different enemy types (data-driven monsters; crab replaces the graybox humanoid)
- [x] Ranged & mage combat styles (ranged spends stamina, mage spends mana)
- [x] Skill tree (Borderlands-style) — 3 branches (Melee/Ranged/Mage), passive bonuses; press **K**
      (combat modifiers + active abilities still to come)
- [ ] Home town scene with a shop (buy/sell)
- [ ] Pause menu & settings (mouse sensitivity, volume)
- [ ] Minimap (fog-of-war)

## How it works

- `ADungeonCrawlerGameMode` — spawns lighting + the dungeon generator on BeginPlay, then drops the
  player into the first room.
- `AFirstPersonCharacter` — eye-height first-person camera with a skeletal-mesh sword that plays a
  swing animation on attack; WASD + mouse-look + sprint via Enhanced Input configured entirely in C++
  (no input assets). Carries stats, health/mana/stamina, and a saved profile.
- `ADungeonGenerator` — grid/tilemap dungeon: rooms (multi-cell) joined by 1-cell-wide L-shaped
  corridors; floor/ceiling/wall tiles are instanced; walls are raised on any cell edge bordering a
  non-floor cell, so doorways appear automatically where corridors meet rooms. Tunables (room count,
  cell size, wall height, prop density, seed) are exposed in the Details panel; **Generate** can be
  re-run in-editor.
- `ADungeonProp` — furniture spawned by the generator; uses imported meshes for the modeled types
  (stool/table/crate) and falls back to graybox cubes otherwise.
- `UStatsComponent` — attributes (STR/INT/DEX/VIT), level/XP, skill & attribute points, and derived
  stats (max health/mana/stamina, damage multipliers).
- `UHealthComponent` / `UResourceComponent` — health+damage (player & monsters) and the regenerating
  mana/stamina pools.
- `ADungeonPlayerController` + `UHUDWidget` — pure-C++ UMG HUD (HP/mana/stamina bars + level), and
  owns the inventory/hotbar widgets and UI-toggle input.
- `UInventoryComponent` + `UInventoryWidget` / `InventorySlotWidget` — stack-based inventory with
  drag-and-drop (move/stack, cross-container, drop-to-world). `ItemDatabase` (`FItemDef` in C++, no
  DataTable) defines items, rarity, value, equip kind, and icon mesh/texture.
- `UHotbarComponent` + `UHotbarWidget` / `HotbarSlotWidget` — 8-slot action bar; selecting a weapon
  slot swaps the held mesh and sets the combat style.
- `ALootChest` + `AItemPickup` — chests roll a loot table into the inventory; pickups display the
  item's real mesh (recolored per item) and auto-collect on overlap.
- `UItemIconSubsystem` — renders each item's mesh to a cached render target once, so UI slots show a
  cheap 3D thumbnail (transparent background; rarity color behind).
- `AMonsterCharacter` + `MonsterDatabase` (`FMonsterDef`) — data-driven enemies (health/speed/damage/
  scale/anim); `AProjectile` for ranged/mage attacks; `ADeathPoof` on death.
- `UDungeonGameInstance` + `UDungeonSaveGame` — persistent player profile across levels and to disk
  (attributes, level/XP, gold, inventory, hotbar, collection log).

The dungeon geometry and props start as code-driven graybox primitives and swap in imported meshes
where available, so the project stays diff-friendly and mesh-agnostic.
