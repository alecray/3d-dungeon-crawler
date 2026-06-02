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

**Controls:** WASD to move, mouse to look, Shift to sprint, Space to jump, LMB to attack (sword swing).

## Core Feature Plan

Built so far (prototype):

- [x] First-person controller (WASD + mouse look, jump)
- [x] Procedurally generated dungeon (rooms + hallways, scattered props)
- [x] Custom floor/wall meshes + textured furniture (stool/table/crate)
- [x] Stylized lighting (Lumen) + amber wall torches
- [x] Health & damage system (player + monsters)
- [x] Melee combat (skeletal sword + swing animation)
- [x] Enemy AI (chase/attack, hit-react) + 3-phase morphing boss
- [x] Player death → level restart

Planned RPG systems (art-independent, in dependency order):

- [ ] Attributes / skills (Strength, Intelligence, Dexterity, Vitality)
- [ ] Health / Mana / Stamina bars (UMG HUD)
- [ ] Save persistence
- [ ] Inventory system with draggable items (Diablo-style)
- [ ] Lootable chests
- [ ] Collection log (rare items)
- [ ] Different enemy types
- [ ] Ranged & mage combat styles
- [ ] Skill tree (Borderlands-style)
- [ ] Home town scene with a shop (buy/sell)

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
- `ADungeonProp` — graybox furniture (chair/table/cabinet/dresser/bookshelf) assembled from cubes.
  Set `MeshOverride` to swap in a finished mesh later.

Everything is code-driven graybox primitives (`/Engine/BasicShapes/Cube`) — no binary Content
assets, so the prototype stays diff-friendly and mesh-agnostic.
