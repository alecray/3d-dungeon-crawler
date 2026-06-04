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

**Controls:** WASD to move, mouse to look, Shift to dash/dodge (costs stamina), Space to jump, LMB to attack (melee swing
/ ranged bolt / spell depending on the equipped weapon), **I** for inventory, **C** for the collection
log, **K** for the skill tree, **1–8** to select the hotbar slot (swaps the held weapon), **Q** to use
your unlocked active ability, **E** to interact with chests / pickups / NPCs / portals, **Esc** for the
pause menu (Resume / Settings / **Dev Menu** / Quit). The full list is also in the pause → Settings panel.

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
- [x] Starting class select (Dark-Souls-style template) — Warrior / Ranger / Mage seed starting
      attributes + weapon at the main menu; no lock-in (level/equip/skill freely afterward)
- [x] Inventory system with draggable items (Diablo-style)
- [x] Lootable chests
- [x] Collection log (rare items)
- [x] 3D item icons (item meshes rendered to cached textures; UI slots + world pickups share them,
      with per-item texture recolor — e.g. health/mana/stamina potions reuse one mesh)
- [x] Equipment & action bar (8 slots, equip weapons, drop items to world)
- [x] Paperdoll equipment (head/amulet/body/gloves/belt/legs/feet + 4 rings) on the inventory screen;
      type-restricted slots, drag to equip / click to unequip, stat bonuses applied while worn
- [x] Different enemy types (data-driven monsters; crab replaces the graybox humanoid)
- [x] Ranged & mage combat styles (ranged spends stamina, mage spends mana)
- [x] Skill tree (Borderlands-style) — 3 branches (Melee/Ranged/Mage), passive bonuses + combat
      modifiers (attack speed, lifesteal, multishot, cost reduction) + capstone active abilities
      (Whirlwind / Volley / Arcane Nova on **Q**); open with **K**
- [x] Home town scene (L_Town) with a shop NPC (buy/sell, with gold shown) + travel portals to/from the
      dungeon (start-room + boss-gated return portals)
- [x] Pause menu & settings (Esc): Resume / Settings / Dev Menu / Quit; mouse sensitivity + master
      volume + a full controls reference, persisted in the profile
- [x] Minimap (fog-of-war)

Boss & encounters:

- [x] Boss encounter sequence — boss spawns when you enter the room (opposite you), entrance doorways
      seal for the fight (reopen on death), and a camera-focus + screen-shake intro cinematic plays
      every encounter (framed on the boss). Return portal on death, placed away from the loot.
- [x] Boss melee telegraph — a red danger disc on the floor (the exact hit radius) brightens through the
      wind-up and flashes on impact; the zone is wide enough that you must **dash** out, not just step back
- [x] Boss fight depth — crab-like scuttle + telegraphed lunge movement (hybrid navmesh: paths around
      cover when line-of-sight is blocked), 3 phases, specials (ground slam, summon adds, projectile
      volley, bubble-pool hazards, enrage, phase-3 shell-retreat), and a phase-1 back weak point (2× dmg)
- [x] Dedicated boss health bar; the boss uses the hermit-crab skeletal mesh (idle/walk anims) with a
      graybox fallback

> **Temporary flag — `ABossMonster::bAbilitiesEnabled` (default OFF).** While the boss animations are
> being finalized, the full fight is gated off: with it off the boss holds in phase 1 and only does the
> standard scuttle/lunge + melee attack (so the attack anim can be tested cleanly) — no phase buffs,
> specials, shell-retreat, or enrage. Flip it on (Details panel → Boss|Debug) to restore the full fight.
> **Remove this flag once the remaining boss anims are in** (tracked in TODO.md).

World & generation:

- [x] Dungeon traps — spike floors, pressure plates, wall-mounted dart shooters (graybox, mesh-swappable)
- [x] Room-type variety — Treasure / Ambush / Rest / Elite / Normal rooms, each with a colored marker light
- [x] Bonfire rest points in Rest rooms — interact (E) to fully heal + refill mana/stamina + checkpoint-save
- [x] Deliberate, themed scenery — per-room décor themes, furniture lined against walls, corner stacks,
      dining sets; scenery avoids traps and never overlaps other scenery
- [x] Bigger rooms + shorter (nearest-neighbor) corridors; atmospheric volumetric fog

Combat feel & VFX (code-driven, no imported art):

- [x] **Dash/dodge** on Shift (replaces sprint) — a short, committed, stamina-costed burst
- [x] **Dark-Souls-leaning combat** — the boss hits hard (~75% of starting HP) but its blow lands on a
      specific animation frame, so it's dodgeable by repositioning; reach is measured from its body edge
- [x] Floating damage numbers; weapon use costs stamina/mana (melee included), with an **insufficient-
      resource bar flash** when you're too tired/low on mana to act
- [x] Game juice — hit-stop on melee impact, camera kick, enemy knockback, low-health red vignette
- [x] VFX pass — impact spark bursts on hits, ambient drifting dust motes, screen damage flash, gold
      level-up burst, dash FOV kick, low-HP chromatic aberration + desaturation
- [x] More VFX — **footstep dust**, **enemy death dissolve** (sink + shrink), **rarity loot beams** on
      dropped items + **pickup sparkle** + **chest open rarity-pop**, **trap telegraph glow** before a
      hazard fires, and a code-driven **boss spawn-in** (ground flare + energy pillar + shard swirl)
- [x] Boss drops rolled loot on death; item-affix naming scaffold ("Sword of the Inferno"-style) in place
- [x] FPS counter (top-right); dev menu (No Clip / God Mode / Reveal Map / Teleport to Boss / Kill Player
      / Teleport Home)

Flow / UX:

- [x] Start screen (main menu on launch) + black-fade scene transitions on every level travel

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
  (attributes, level/XP, gold, inventory, hotbar, collection log, and which boss intros have played).
- `ABossMonster` (on `AMonsterCharacter`) — the hermit-crab boss: skeletal mesh + idle/walk anims,
  scuttle/lunge movement, 3 phases, the specials above, and a phase-1 back weak point.
- `ABossArena` + `ABossDoor` — the encounter manager: a room trigger that spawns the boss on entry,
  seals the doorways with rising barriers, runs the intro camera (`UBossIntroCameraShake`) on the first
  kill, and cleans up its boss/doors/portal on regenerate.
- `ADungeonTrap` (spike floor / pressure plate / dart shooter) + `ABubbleHazard` — graybox hazards with
  mesh swap-in points; the generator tags rooms with a type (`ERoomType`) and a décor theme for
  deliberate scenery placement.
- `ADamageNumber`, `AImpactBurst`, `AAmbientDust`, `UHitCameraShake` — code-driven combat/ambient VFX
  (floating numbers, hit sparks, drifting dust, screen kick).
- `UMainMenuWidget`, `ULowHealthVignetteWidget`, `UFpsCounterWidget`, `UBossHealthBarWidget`,
  `UMinimapWidget` — pure-C++ HUD/menu widgets; `ADungeonPlayerController` drives the start screen,
  black-fade scene transitions, the dev menu, and the boss-room teleport.

The dungeon geometry and props start as code-driven graybox primitives and swap in imported meshes
where available, so the project stays diff-friendly and mesh-agnostic.
