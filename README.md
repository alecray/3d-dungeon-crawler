# 3d-dungeon-crawler

First-person, procedurally generated dungeon crawler. **UE 5.7, pure C++** (no Blueprints), all
graybox so Blender assets can drop in later without code changes.

## Running

> Path placeholders used below: **`<UE>`** = your Unreal Engine 5.7 install root (the folder containing
> `Engine\`, e.g. `C:\Program Files\Epic Games\UE_5.7`); **`<PROJECT>`** = this repository's root (the
> folder containing `DungeonCrawler.uproject`). Substitute your own locations.

1. Build the editor target:
   ```
   "<UE>\Engine\Build\BatchFiles\Build.bat" DungeonCrawlerEditor Win64 Development -project="%CD%\DungeonCrawler.uproject" -waitmutex
   ```
2. Open `DungeonCrawler.uproject` in Unreal Editor, then **Play**. The game mode builds the whole
   world at runtime — any empty level works; no map asset is needed.

**Controls:** WASD to move, mouse to look, Shift to dash/dodge (costs stamina), Space to jump, LMB to attack (melee swing
/ ranged bolt / spell depending on the equipped weapon), **I** for inventory, **C** for the collection
log, **K** for the skill tree, **1–8** to select the hotbar slot (swaps the held weapon), **Q** to use
your unlocked active ability, **E** to interact with chests / pickups / NPCs / portals, **Esc** for the
pause menu (Resume / Settings / **Dev Menu** / Quit). The full list is also in the pause → Settings panel.

## Tools (`tools/`)

Headless Python scripts for import pipelines, asset authoring, and level setup. See
**[`tools/README.md`](tools/README.md)** for full usage of every script:

| Script | What it does |
|---|---|
| `import_meshes.py` | FBX import pipelines (static, skeletal, anim, batch folder) |
| `make_interchange_presets.py` | Builds the Content Browser drag-drop pipeline preset assets |
| `build_town.py` | Authors `L_Town` as placed actors (sky, forest, hub stations, scatter) |
| `make_water_material.py` | Builds `M_PondWater` (clarity-first stylized pond material) |
| `make_card_material.py` | Builds `M_Card` (unlit masked material for blackjack card faces) |
| `place_blackjack.py` | Places `ABlackjackTable` into `L_Town` and saves the level |
| `add_emissive_to_mbase.py` | Adds `EmissiveStrength` param to `M_Base` (default 0, safe to re-run) |
| `make_wisp_material.py` | Builds `M_Wisp` + `MI_Wisp` (unlit emissive with `EmissiveStrength` param); assigns to `SK_Wisp` |
| `disable_nanite.py` | Turns Nanite off on all static meshes (not needed for this low-poly project) |

## Core Feature Plan

Built so far:

- [x] First-person controller (WASD + mouse look, jump, dash/dodge on Shift)
- [x] Procedurally generated dungeon (rooms + hallways, scattered props)
- [x] Custom floor/wall meshes + textured furniture (stool/table/crate); floor mesh reused as the
      ceiling (underside shown for separate texturing); walls alternate facing for variety
- [x] Stylized lighting (Lumen) + amber wall torches
- [x] Health & damage system (player + monsters)
- [x] Melee combat (skeletal sword + swing animation)
- [x] Enemy AI (chase/attack, hit-react, backstab bonus) + a telegraphed hermit-crab boss
- [x] Player death → respawn in town with a scaling gold toll (death cost rises with player level)
- [x] Attributes (Strength, Intelligence, Dexterity, Vitality) + level/XP progression (XP on kills,
      melee scales with Strength)
- [x] Health / Mana / Stamina bars (pure-C++ UMG HUD); stamina drives the dash + weapon use
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
      dungeon (start-room + boss-gated return portals); interacting with the enter-dungeon portal opens a
      **map/tier select menu** — pick a map (one dungeon currently) and a **tier (0–3)**, then **Go**
      travels you in at that tier (stored on the Game Instance)
- [x] Pause menu & settings (Esc): Resume / Settings / Dev Menu / Quit; mouse sensitivity + master
      volume + a full controls reference, persisted in the profile
- [x] Minimap (fog-of-war)

Boss & encounters:

- [x] Boss encounter sequence — boss spawns when you enter the room (opposite you), entrance doorways
      seal for the fight (reopen on death), and a camera-focus + screen-shake intro cinematic plays
      every encounter (framed on the boss). Return portal on death, placed away from the loot.
- [x] Boss melee telegraph — a red danger disc marking a **forward strike zone** (pushed out along the
      boss's facing to where the blow actually lands, not a ring under its body); it brightens through the
      wind-up and flashes on impact, and is wide enough that you must **dash** out, not just step back. The
      disc + the real hit are frozen at swing start so they stay in sync, and the boss self-illuminates a
      touch so the big crab reads in the dark
- [x] Boss fight depth — crab-like scuttle + telegraphed lunge movement (hybrid navmesh: paths around
      cover when line-of-sight is blocked), Dark-Souls-style multi-phase "lives" (at 0 HP the boss heals
      to full and advances a phase rather than dying — only the last phase ends the fight), and a
      **phase star indicator** next to the boss health bar showing lives remaining (e.g. ★★ → ★ → dead).
      **Tier 0 has 2 phases**: phase 1 is the baseline fight; phase 2 adds a **ground slam** — a large
      telegraphed AOE the player must dodge by **jumping** (airborne at impact = safe). A back weak point
      (2× damage) rewards flanking in phase 1.
- [x] **Boss tiers 0–3** — interacting with the town portal lets you choose a tier before entering. Tier 0
      is the baseline. Tiers 1–3 scale the boss's health and damage for higher challenge. Unique phase
      content and special abilities for tiers 1–3 are **planned but not yet implemented**; the current
      tier system is stat-scaling only.
- [x] Dedicated boss health bar; the boss uses the hermit-crab skeletal mesh (idle/walk anims) with a
      graybox fallback

World & generation:

- [x] Dungeon traps — spike floors, pressure plates, wall-mounted dart shooters (graybox, mesh-swappable)
- [x] Room-type variety — Treasure / Ambush / Rest / Elite / Normal rooms, each with a colored marker light
- [x] Bonfire rest points in Rest rooms — interact (E) to fully heal + refill mana/stamina + checkpoint-save
- [x] Deliberate, themed scenery — per-room décor themes, furniture lined against walls, corner stacks,
      dining sets; scenery avoids traps and never overlaps other scenery
- [x] Bigger rooms + shorter (nearest-neighbor) corridors; atmospheric volumetric fog

Combat feel & VFX (code-driven, no imported art):

- [x] **Dash/dodge** on Shift (replaces sprint) — a short, committed, stamina-costed burst with
      **i-frames** (brief invulnerability at the start, Souls-style roll-through; `DashIFrameDuration`)
- [x] **Dark-Souls-leaning combat** — the boss hits hard (~75% of starting HP) but its blow lands on a
      specific animation frame, so it's dodgeable by repositioning; reach is measured from its body edge
- [x] Floating damage numbers that spawn **at the impact point** on the enemy and linger; a **"-N" popup**
      pops over your HP bar and drifts off when you take damage; weapon use costs stamina/mana (melee
      included), with an **insufficient-resource bar flash** when you're too tired/low on mana to act
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
- `AFirstPersonCharacter` — eye-height first-person camera with a skeletal-mesh weapon; WASD +
  mouse-look + a stamina-costed **dash/dodge** (Shift, with i-frames) via Enhanced Input configured
  entirely in C++ (no input assets). Owns the player's components and wires input to them; carries the
  saved profile and applies the chosen starting class loadout (`ApplyClassLoadout`).
- `UCombatComponent` / `UFishingComponent` — attack execution (melee/ranged/mage swings + bolts, the Q
  abilities, hit-stop, camera kick; the melee hit lands partway through the swing) and town fishing (rod
  swap, poses, status line), both split out of the player pawn. **Gold** lives in the GameInstance profile
  (the pawn just forwards to it); **dev cheats** (No Clip / God Mode / Kill) live on `ADungeonPlayerController`.
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
- `ALootChest` + `AItemPickup` — chests roll a loot table into their grid (and pop a rarity-colored
  burst on first open); world pickups display the item's real mesh, emit a **rarity loot beam**, are
  collected with **[E]**, and spark in the rarity color on pickup.
- `UItemIconSubsystem` — renders each item's mesh to a cached 3D thumbnail (transparent background;
  rarity color behind) shared by UI slots and world pickups. **Icons are generated automatically — there
  is no manual step for new items.** The first time an item's icon is needed, the subsystem renders its
  mesh and bakes a PNG to `Content/Images/ItemIcons/<ItemId>.png`, then reuses that file. To force a
  re-bake (e.g. after changing a mesh or the icon lighting), run the console command **`dc.IconRebuild`**
  (deletes the baked PNGs + drops the cache); tune the look with the `dc.IconKeyLight` /
  `dc.IconFillLight` / `dc.IconHeadLight` CVars, then `dc.IconRebuild` and reopen the inventory.
- `AMonsterCharacter` + `MonsterDatabase` (`FMonsterDef`) — data-driven enemies (health/speed/damage/
  scale/anim); `AProjectile` for ranged/mage attacks; `ADeathPoof` on death.
- `UDungeonGameInstance` + `UDungeonSaveGame` — persistent player profile across levels and to disk
  (attributes, level/XP, gold, inventory, hotbar, collection log).
- `ABossMonster` (on `AMonsterCharacter`) — the hermit-crab boss: skeletal mesh + idle/walk/attack anims,
  scuttle/lunge movement, an attack that lands on a specific frame inside a wide telegraphed danger zone,
  multi-phase "lives" (heals to full and advances a phase at 0 HP), a **ground-slam** AOE in phase 2
  (jump to survive), a star indicator tracking lives remaining, a back weak point (2× damage in phase 1),
  and **tier scaling** (health/damage multiplied by tier; tiers 1–3 stat-scale only — unique phase content
  for those tiers is planned).
- `ABossArena` + `ABossDoor` — the encounter manager: a room trigger that spawns the boss on entry,
  seals the doorways with rising barriers, runs the intro camera (`UBossIntroCameraShake`) every
  encounter, drops the boss's loot on death, and cleans up its boss/doors/portal on regenerate.
- `ABossSpawnVFX` / `AAttackTelegraph` — the boss's code-driven spawn-in effect, and the red floor disc
  that marks (and flashes on) its incoming hit.
- `ABonfire` — a Rest-room rest point: `[E]` to fully heal, refill mana/stamina, and checkpoint-save.
- `ABlackjackTable` + `UBlackjackWidget` — a town minigame (placed in L_Town): a textured 3D card table;
  cards deal as **real card-face images** on the felt (dealer's hole card face-down until reveal), newest
  card on the left. Pressing **E** pulls the player to a fixed seat for a consistent view. The rules engine
  lives in the table; the UI is a left-side control panel (Gold/Bet/buttons), a bottom-center status line,
  and 3D **"Dealer N / You N"** labels beside each row. Card faces decode from PNGs at runtime onto a
  masked `M_Card` material — see `tools/README.md` for the headless material/placement scripts.
- `AFishingHole` — a town minigame (prototype): cast with `[E]`, reel when the bobber shakes to land a
  random fish (weighted catch table → inventory). Graybox water/bobber/fish with mesh swap-in points.
- `CharacterClass.h` — the starting-archetype loadouts (Warrior/Ranger/Mage: a stat spread + a weapon).
- `ADungeonTrap` (spike floor / pressure plate / dart shooter) + `ABubbleHazard` — graybox hazards with
  mesh swap-in points; the generator tags rooms with a type (`ERoomType`) and a décor theme for
  deliberate scenery placement.
- `ADamageNumber`, `AImpactBurst`, `AAmbientDust`, `UHitCameraShake` — code-driven combat/ambient VFX
  (big floating numbers, dark-blood hit sparks, drifting dust, screen kick; `AImpactBurst::Configure`
  tunes count/flash so the same actor does footstep dust, hit spray, and pickup sparkles).
- `UMainMenuWidget`, `ULowHealthVignetteWidget`, `UFpsCounterWidget`, `UBossHealthBarWidget`,
  `UMinimapWidget` — pure-C++ HUD/menu widgets; `ADungeonPlayerController` drives the start screen,
  black-fade scene transitions, the dev menu, and the boss-room teleport.

The dungeon geometry and props start as code-driven graybox primitives and swap in imported meshes
where available, so the project stays diff-friendly and mesh-agnostic.

Source under `Source/DungeonCrawler/` is organized into subfolders — `Core/` (module, game modes, game
instance, player controller), `Player/`, `Components/`, `Enemies/`, `Dungeon/`, `Items/`, `Town/`,
`UI/`, `VFX/`. Each is added as an include path in `DungeonCrawler.Build.cs`, so headers are still
included by bare filename.

## Code conventions

- **Reference assets with `TSoftObjectPtr<T>`, never raw path strings.** For any reference to a content
  asset (meshes, anims, textures, materials, Niagara, etc.), use a typed `TSoftObjectPtr<T>` rather than
  an `FString`/`const TCHAR*` path that you later `FSoftObjectPath(...).TryLoad()`. Prefer it over a hard
  `TObjectPtr<T>` too unless the asset must always be loaded. Why: it's type-safe (the wrong asset class
  won't compile), the reference is visible to the cooker and the reference viewer, an `EditAnywhere` field
  becomes a filtered asset picker in the Details panel, and per-instance/Blueprint overrides participate
  in the editor's redirector fix-up on move/rename — so assets don't silently break when relocated.
  Pattern: an `EditAnywhere` `TSoftObjectPtr<T>` field defaulted to the conventional path
  (`= TSoftObjectPtr<T>(FSoftObjectPath(TEXT("/Game/...")))`), resolved at use with `.LoadSynchronous()`
  / `.Get()`. The data registries (`ItemDatabase`, `MonsterDatabase`) and asset-holding actors
  (`AFirstPersonCharacter`, `ABossMonster`, `ADungeonGenerator`, `ADungeonProp`, …) follow this.
  The only sanctioned exceptions are **engine built-ins** (e.g. `/Engine/BasicShapes/Cube`) loaded inline
  as graybox placeholders, and **constructor-time `ConstructorHelpers::FObjectFinder`** (which can only
  take a literal path) — both are fine as-is.
