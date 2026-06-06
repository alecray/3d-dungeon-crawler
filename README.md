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

## Importing meshes (`Tools/import_meshes.py`)

Reusable FBX **import pipelines** so you never have to re-toggle import settings between static and
skeletal meshes again. It defines one locked-in preset for each type (static / skeletal), plus
single-file and batch helpers. Defaults suit this project: Blender FBX exports (normals imported, not
recomputed), Nanite **off** on static meshes, materials + textures imported, auto-collision on statics,
morph targets on skeletals, optional skeleton reuse. All the shared knobs live in one `CONFIG` block at
the top of the script — set your preferences once and both pipelines follow.

**In-editor** (Output Log → Cmd bar set to *Python*):

```python
import importlib, import_meshes; importlib.reload(import_meshes)
import_meshes.import_static(r"<PROJECT>\blends\world\SM_Tree.fbx", "/Game/World")
import_meshes.import_skeletal(r"...\SK_Staff.fbx", "/Game/Weapons/Staff",
                              skeleton="/Game/Weapons/Staff/SKEL_Staff")  # reuse a skeleton (optional)
import_meshes.import_folder(r"...\blends\world", "/Game/World", kind="static")  # batch a folder
```

(For `import import_meshes` to resolve, add `Tools/` under **Project Settings → Plugins → Python →
Additional Paths**, or run the full path once: `py "<PROJECT>/Tools/import_meshes.py"`.)

**Headless** (editor closed) — **use forward slashes**, backslashes mangle the path (`\3` gets eaten):

```
"<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<PROJECT>/DungeonCrawler.uproject" ^
    -run=pythonscript -script="<PROJECT>/Tools/import_meshes.py static <PROJECT>/blends/.../SM_Foo.fbx /Game/World" ^
    -unattended -nopause -nosplash
```

`-script` args: `static <fbx> <dest>` · `skeletal <fbx> <dest> [skeletonAssetPath]` ·
`folder <srcDir> <dest> <static|skeletal>`. (Headless argv splits on spaces, so the **paths themselves
must not contain spaces**; for spaced paths call the functions from a small runner `.py` instead.)

**Adding another import pipeline** (e.g. animation-only, or a variant with different settings):

1. Add a builder that returns a configured `unreal.FbxImportUI`, mirroring `build_static_options()` /
   `build_skeletal_options()`. Set the type (`mesh_type_to_import` + `import_as_skeletal`/`import_mesh`/
   `import_animations`) and tune the per-type data object (`static_mesh_import_data` /
   `skeletal_mesh_import_data` / `anim_sequence_import_data`). Use the `_set()` helper — it logs-and-skips
   any property name that differs in this engine build instead of crashing.
2. Add a thin wrapper like `import_static()` that calls `_run(fbx, dest, your_options())`.
3. Put any knob you want shared across pipelines in the `CONFIG` dict at the top.
4. (Optional) add a verb to `_main()` so it's callable headlessly.

> Note: this uses the legacy `FbxImportUI` path (stable + scriptable in UE 5.7). If a setting is ever
> ignored, the script logs which property it skipped — switch that pipeline to the Interchange API if needed.

## Building the town (`Tools/build_town.py`)

The town hub level (`/Game/Maps/Town/L_Town`) is **authored by a headless Python script**, not spawned
at runtime — `ATownGameMode` only ensures fallback lighting. Running the script (re)builds the whole
clearing as placed, editable actors in `L_Town`: open-sky backdrop (SkyAtmosphere + atmosphere sun +
real-time-capture SkyLight + height fog), the **invisible boundary ring** that keeps the player in, the
enclosing forest (tree line + deeper trees), a distant vista ring, the hub stations (Shop / Blackjack /
Fishing / enter-dungeon Portal / Bonfire), and scatter scenery.

Run it with the editor **closed**:

```
"<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<PROJECT>/DungeonCrawler.uproject" ^
    -run=pythonscript -script="<PROJECT>/Tools/build_town.py" -unattended -nopause -nosplash
```

Key things to know:

- **One knob sizes everything:** `CLEARING_HALF` (cm, default `1100` ≈ 22 m square) drives the boundary,
  tree ring, station spacing, and scatter. Lower it to shrink the clearing, then re-run.
- **Everything is anchored to the `PlayerStart`** and filed under the World Outliner folder **`TownHub`**.
- **It's destructive-but-scoped:** on each run it deletes only actors already in the `TownHub` folder and
  respawns them — so it won't touch hand-placed actors you keep outside that folder, but **manual edits to
  `TownHub` actors are lost on re-run.** Save `L_Town` after a run; only re-run when you want to regenerate.
- **The boundary** is four invisible `BlockAll` boxes (collision on, visibility off), labeled
  **`Boundary1`–`Boundary4`**, hidden behind the tree line (the trees themselves have no collision). Toggle
  the viewport collision view (Alt+C) to see them. See the "town clearing" note for resizing.

(Other `Tools/*.py` headless scripts follow the same invocation pattern — e.g. `disable_nanite.py`,
`place_blackjack.py`, `make_card_material.py`, `add_emissive_to_mbase.py`.)

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
- [x] Player death → level restart
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
      dungeon (start-room + boss-gated return portals)
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
- `AFirstPersonCharacter` — eye-height first-person camera with a skeletal-mesh weapon; the melee hit
  lands partway through the swing. WASD + mouse-look + a stamina-costed **dash/dodge** (Shift) via
  Enhanced Input configured entirely in C++ (no input assets). Carries stats, health/mana/stamina, a
  saved profile, and applies the chosen starting class loadout (`ApplyClassLoadout`).
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
- `UItemIconSubsystem` — renders each item's mesh to a cached render target once, so UI slots show a
  cheap 3D thumbnail (transparent background; rarity color behind).
- `AMonsterCharacter` + `MonsterDatabase` (`FMonsterDef`) — data-driven enemies (health/speed/damage/
  scale/anim); `AProjectile` for ranged/mage attacks; `ADeathPoof` on death.
- `UDungeonGameInstance` + `UDungeonSaveGame` — persistent player profile across levels and to disk
  (attributes, level/XP, gold, inventory, hotbar, collection log).
- `ABossMonster` (on `AMonsterCharacter`) — the hermit-crab boss: skeletal mesh + idle/walk/attack anims,
  scuttle/lunge movement, an attack that lands on a specific frame inside a wide telegraphed danger zone,
  and (gated behind `bAbilitiesEnabled`, off while anims are finalized) 3 phases + specials + a back weak point.
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
  masked `M_Card` material — see `Tools/` for the headless material/placement scripts.
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
