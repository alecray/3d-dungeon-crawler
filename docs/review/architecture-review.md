# Architecture & Project Structure Review

Project: **3d-dungeon-crawler** (Unreal Engine 5.7, C++) — first-person, procedurally generated, Dark Souls-inspired dungeon crawler.
Scope: project structure & architecture vs. UE C++ best practices. Read-only review of graybox-stage code.
Date: 2026-06-07.

---

## Summary (most important takeaways)

- **Overall: a well-organized, genuinely C++-first project that follows most UE conventions.** This is well above the typical indie/hobby UE codebase.
- **Module layout is minimal-but-correct:** one `Runtime` module, clean `.Build.cs`/`.Target.cs`, no `Engine.h` kitchen-sink includes.
- **Memory/ownership is exemplary:** `TObjectPtr` used in ~43 headers, zero raw `new UObject`, no GC-unsafe raw-pointer `UPROPERTY` arrays.
- **Data-driven design is strong but unconventional:** monsters/items/affixes live in **code registries** (`MonsterDatabase`, `ItemDatabase`, `AffixDatabase`) instead of `DataTable`/`DataAsset` — a deliberate, diff-friendly choice that works, but bypasses designer-facing tooling.
- **Biggest structural smell: a flat `Source/DungeonCrawler/` with 121 files** (60 cpp / 61 h) and no sub-folders — navigability will degrade as it grows.
- **`DungeonGenerator` (1381 lines) and `FirstPersonCharacter` (1363 lines) are doing a lot** — internally well-decomposed, but candidates for extraction (the player is a multi-responsibility hub).
- **AI uses Tick-based steering with a stock `AAIController`** — no Behavior Tree / StateTree. Fine for graybox; flag it as a deliberate scale ceiling.
- **UI is 100% pure-C++ `UUserWidget`** (no WBP assets) — consistent with the "no Blueprint" mandate and impressively disciplined.
- **Tooling & pipeline are a highlight:** the Blender→FBX→`import_meshes.py` path, Interchange preset config, and the commit-count version hook are all clean and documented.
- **Minor content hygiene issues:** stray `Content/Maps/DungeonTest/NewMap.umap`, an empty `Content/Collections/` folder, and `EquipSlot`/affix scaffolding that is defined but not yet wired.

---

## What's done well

### Module & build setup
- `DungeonCrawler.uproject:6-26` declares a single `Runtime` module plus only the plugins actually used (EnhancedInput, PythonScriptPlugin). No plugin bloat.
- `Source/DungeonCrawler/DungeonCrawler.Build.cs:9-24` lists exactly the dependencies in use (AIModule, NavigationSystem, UMG, Niagara, ImageWrapper, GameplayTags). `PCHUsage = UseExplicitOrSharedPCHs` is the recommended default.
- `Source/DungeonCrawler.Target.cs:9-10` correctly pins `BuildSettingsVersion.Latest` and `EngineIncludeOrderVersion.Latest` — proper modern-include-order hygiene.
- `EngineAssociation` is `5.7`, matching the workspace `CLAUDE.md` mandate.

### Header hygiene & include discipline
- **No `#include "Engine.h"`** anywhere (the classic UE anti-pattern). Headers lean on `CoreMinimal.h` + forward declarations.
- `FirstPersonCharacter.h:8-26` forward-declares ~15 types instead of including them — textbook compile-time hygiene.
- The handful of header-level includes of other gameplay headers are all *justified* by a struct/enum the header genuinely embeds, e.g. `DungeonSaveGame.h:5-6` (needs `FCoreAttributes`, `FInventorySlot`), `EquipmentComponent.h:5` (`FItemDef`), `BossMonster.h:4` (base class). Each carries an inline comment naming the symbol it needs.

### Memory & ownership (GC correctness)
- `TObjectPtr<>` is used for object members across ~43 headers — the UE5-preferred form.
- **Zero raw `new U*`**; object creation goes through `CreateDefaultSubobject` / `NewObject` (28 cpp files).
- Spawned-actor bookkeeping is GC-safe: `DungeonGenerator.h:260-261` keeps `UPROPERTY() TArray<TObjectPtr<AActor>> SpawnedActors;` so regenerate can clear them without dangling pointers. No raw-pointer `UPROPERTY` arrays found.

### Gameplay-framework usage
- Clean separation: `ADungeonCrawlerGameMode` (`DungeonCrawlerGameMode.h`) builds the world; `ATownGameMode` subclasses it and overrides `BuildWorld()` (`TownGameMode.h:4`) — good use of the GameMode hierarchy with a virtual build hook.
- `DungeonGameInstance` owns the persistent profile + `DungeonSaveGame`, the correct home for cross-level state (`DefaultEngine.ini:3` wires it as `GameInstanceClass`).
- Composition over inheritance on the player: Health/Resource/Stats/Inventory/Hotbar/SkillTree/Equipment are all `UActorComponent`s on `AFirstPersonCharacter` (`FirstPersonCharacter.h:200-222`), each with a const accessor (`:65-72`). This is exactly the component pattern UE encourages.
- `UHealthComponent` (`HealthComponent.h`) is a clean, reusable, decoupled component using multicast delegates (`:10-13`) rather than hard references back to the owner — shared by player and monsters.

### Decomposition of the big systems
- Despite its size, `DungeonGenerator.cpp` is **not** a god-blob: it's a clear pipeline of small private steps — `PlaceRooms`/`CarveCorridors`/`BuildGeometry`/`ScatterProps`/`ScatterMonsters`/`ScatterChests`/`ScatterTraps`/`SetupBossEncounter` (`DungeonGenerator.h:277-298`). Grid helpers are tight inline const methods (`:270-274`).
- Geometry is instanced (`UInstancedStaticMeshComponent` for floor/ceiling/wall/torch, `:234-244`) — the right perf call for tile-based geometry.

### Const-correctness & API design
- Accessors are consistently `const` (17 const methods in `FirstPersonCharacter.h`, 16 each in `StatsComponent.h` / `DungeonGenerator.h`).
- `UPROPERTY(EditAnywhere)` tunables carry `meta=(ClampMin/ClampMax)` throughout (`DungeonGenerator.h:87-229`, `MonsterCharacter.h:90-95`) and are grouped with `Category="Dungeon|Grid"` style sub-categories — designer-friendly even though there's no designer.
- `Generate()` is exposed `UFUNCTION(CallInEditor)` (`DungeonGenerator.h:60-61`) for in-editor iteration — a nice touch.

### Data-driven (with a caveat — see deviations)
- Item/monster/affix definitions are real data structs (`FMonsterDef`, `FItemDef`, `FItemAffix`, `FItemBonuses`) with helper namespaces for lookup + weighted rolls (`MonsterTypes.h:37-45`, `ItemTypes.h:191-199`). The *shape* is data-driven; only the *storage* is code (see below).
- Forward-looking modeling: affixes, elemental damage, per-instance `Affixes` on inventory slots (`ItemTypes.h:139-155`) are designed in now with honest `// SCAFFOLDING` comments about what's not yet wired.

### Versioning, tooling, git/LFS
- Commit-count version stamp implemented exactly per the workspace convention: `Config/DefaultGame.ini:5` (`ProjectVersion=0.1.144`) auto-bumped by `.githooks/pre-commit`, with the full hook set (`post-commit`/`post-checkout`/`post-merge`/`pre-push`) present.
- Git LFS configured for all binary/art types in `.gitattributes` (uasset, umap, fbx, blend, png, tga, wav, uexp, ubulk).
- `.gitignore` correctly excludes `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`, the runtime-baked `Content/Images/ItemIcons/`, and `__pycache__/`.
- The art pipeline (`Tools/import_meshes.py`) is well-documented, defensive (`_set()` swallows renamed-property errors, `:58-65`), supports in-editor *and* headless invocation, and centralizes settings in one `CONFIG` block (`:39-53`). The Interchange preset stacks in `DefaultEngine.ini:54-55` are a thoughtful workflow optimization.
- `Content/Developers/ajray/` exists but is **not** committed — correct handling of the Developers scratch folder.

---

## What deviates from best practices

### 1. Flat source folder — no sub-module / sub-folder structure
`Source/DungeonCrawler/` holds **121 files** in one directory: gameplay actors, components, widgets, VFX, data types, subsystems all intermixed. UE convention for a project this size is sub-folders (e.g. `Player/`, `Enemies/`, `Dungeon/`, `Items/`, `UI/`, `Data/`, `VFX/`). At 60 cpp files this is the single most noticeable structural deviation. (UE source folders are flat namespace-wise, so sub-folders are free — no `.Build.cs` changes needed.)

### 2. Single module for everything (including editor-y/import-y code)
Everything is one `Runtime` module. As the project grows, conventional layout would split out at least:
- an **Editor module** for anything editor-only, and
- the icon-baking / `ItemIconSubsystem` path, which is tightly coupled to editor/SceneCapture concerns (`ItemIconSubsystem.cpp`).
Not urgent at graybox stage, but worth planning before the file count doubles.

### 3. `FirstPersonCharacter` is a multi-responsibility hub (1363 lines)
`FirstPersonCharacter.cpp` is the second-largest file and the class owns: input setup, melee/ranged/mage combat, dash, hit-stop/camera-kick game-feel, fishing flow, gold/shop, dev cheats, save/persist, class loadout, and resource-deny feedback (see the surface area in `FirstPersonCharacter.h:77-130, 334-439`). Several of these are extractable:
- **Combat** (melee sweep + projectile fire + hit-stop) → a `UCombatComponent`.
- **Fishing** (`:77-90`, plus `bFishing`/pose state `:435-439`) → already driven by `AFishingHole`; the player-side state could move into a small component.
- **Dev cheats** (`DevToggleNoClip`/`DevToggleGodMode`/`DevKill`, `:112-114`) → a cheat manager (`UCheatManager` is the canonical UE home).
This is "feature-rich pawn" rather than a bug, but it's the class most at risk of becoming a true god object.

### 4. Code registries instead of DataTable/DataAsset
`MonsterDatabase`/`ItemDatabase`/`AffixDatabase` (`MonsterTypes.h:37-45`, `ItemTypes.h:176-199`) store all definitions in C++. The header comments acknowledge this is deliberate ("Defined in C++ so items stay diff-friendly; swap to a DataTable later if desired", `ItemTypes.h:188-190`). Trade-offs to flag:
- **Pros (real):** text-diffable in git, no asset-merge pain, refactor-safe, no LFS churn.
- **Cons (best-practice deviation):** no in-editor authoring, no hot-reload of balance numbers without a recompile, no designer access, and content paths are stored as raw `FString` (`MonsterTypes.h:27-30`, `ItemTypes.h:129-136`) rather than `TSoftObjectPtr<>`, so a renamed asset fails silently at runtime instead of being caught by the reference system. If this stays code-based, consider `TSoftObjectPtr<USkeletalMesh>`/`TSoftClassPtr<>` for the asset fields to get validation and the cook-reference graph.

### 5. AI: Tick-based steering, stock AIController, no BT/StateTree
`MonsterCharacter.cpp:54-55` auto-possesses a plain `AAIController`; chase/attack logic runs in `AMonsterCharacter::Tick` with a periodic re-path (`MonsterCharacter.h:201-203`). The boss adds a `TickCustomChase` virtual override hook (`MonsterCharacter.h:140`). This is pragmatic and fine for a few enemy archetypes, but it's a deliberate ceiling: Behavior Trees / StateTree are the UE-conventional home for anything beyond chase-and-swing, and per-frame steering in `Tick` for every monster has a cost as counts grow. Flag as a scale decision, not a defect.

### 6. Minor content-organization issues
- `Content/Maps/DungeonTest/NewMap.umap` is a stray default-named map (committed) — should be removed or renamed.
- `Content/Collections/` is committed but empty.
- `Content/` mixes singular and plural top-level folders (`Consumable`, `Cards`, `Collections`, `Enemies`, `Weapons`, `World`, `Tools`, `Pipelines`) without a single convention; `Enemies/` has the cleaner `Bosses/`+`Regular/` split that the rest could follow.
- Asset prefix conventions are good and consistent where present (`SK_`, `SKEL_`, `SM_`, `MI_`, `M_`, `T_`, `A_`, `L_`, `PL_`).

### 7. Scaffolding defined but inert (intentional, but worth tracking)
`EEquipSlot` paperdoll slots (`ItemTypes.h:38-50`), `FItemAffix` rolling, and elemental `FlatBonusDamage` (`:79-80`) are modeled but not yet applied in combat — clearly marked `// SCAFFOLDING`. This is honest and low-risk, but it's dead surface area that a reader must mentally filter; keep the TODO list authoritative so it doesn't rot.

### 8. Small nits
- `FirstPersonCharacter.h:9-22` declares `USkeletalMesh`, `UAnimSequence`, and `class CharacterClass` enum twice (duplicate forward declarations on lines 10/21 and 11/22). Harmless, but tidy up.
- `FirstPersonCharacter.h:130-181` files staff/crossbow/fishing-rod meshes all under `Category="Sword"` — the category label no longer matches the contents now that it holds non-sword weapons; rename to `Category="Weapon"` for the Details panel.

---

## Comparison to typical small indie UE dungeon-crawler projects

- **Better than typical:** Most hobby/indie UE RPGs are Blueprint-heavy with thin C++; this project is the inverse (pure C++ incl. all UI and Enhanced Input config in code) and has *no* Blueprint logic — fully meeting the workspace's "C++, never Blueprint" rule. Memory hygiene (TObjectPtr, no raw new) and include discipline are also better than the median.
- **In line with norms:** single Runtime module for an early project; GameMode/GameInstance/Component composition; instanced static meshes for tile geometry; LFS for art.
- **Below norms in one place:** comparable projects usually use `DataTable`/`PrimaryDataAsset` for item/enemy data and Behavior Trees for AI. This project trades those for code registries and Tick-steering — defensible at graybox scale, but the two most likely future refactors.

---

## Prioritized recommendations

**P1 — do soon, low effort, high payoff**
1. **Introduce source sub-folders** (`Player/ Enemies/ Dungeon/ Items/ UI/ Data/ VFX/ World/`). Pure file moves; no build changes. Single biggest readability win.
2. **Delete/rename `Content/Maps/DungeonTest/NewMap.umap`** and remove the empty `Content/Collections/` folder.
3. **Fix the small header nits** in `FirstPersonCharacter.h`: drop duplicate forward declarations (lines 21-22) and rename the `"Sword"` UPROPERTY category to `"Weapon"`.

**P2 — plan before the codebase doubles**
4. **Extract responsibilities off `AFirstPersonCharacter`**: a `UCombatComponent` (melee+ranged+mage+hit-stop) and move dev cheats into a `UCheatManager`. Keeps the pawn a thin coordinator.
5. **Convert content asset paths to soft references** (`TSoftObjectPtr<USkeletalMesh>` / `TSoftClassPtr<>`) in `FMonsterDef` and `FItemDef`, even while keeping code registries — gains reference validation and cook-graph awareness over raw `FString` paths.
6. **Adopt one Content/ naming convention** (pluralize consistently; replicate the `Enemies/Bosses`+`Regular` sub-split pattern across categories).

**P3 — revisit when scope demands**
7. **Split an Editor module** out for the icon-baking / SceneCapture path and any future editor utilities.
8. **Move enemy AI to Behavior Tree / StateTree** if/when more than ~3 enemy archetypes or non-trivial behaviors are needed; profile per-monster `Tick` steering as counts climb.
9. **Decide the long-term data home**: keep code registries (and lean into validation per #5) *or* migrate to DataTables/DataAssets — but make it a conscious call before content volume makes migration painful.

---

*No source code was modified for this review.*
