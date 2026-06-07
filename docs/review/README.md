# Project Review — Index & Summary

_Generated 2026-06-07. A three-angle review of the 3D low-poly dungeon-crawler project: architecture, in-code documentation, and gameplay/genre improvements. Each angle has its own detailed report (linked below); this page is the skimmable overview._

## The three reports
- **[Architecture & structure](architecture-review.md)** — project/module layout, C++ patterns, data design, pipeline & tooling vs UE best practices.
- **[Documentation pass](documentation-pass.md)** — record of the comment-only documentation added across `Source/`, plus refactor candidates spotted along the way.
- **[Improvements & genre comparison](improvements-and-genre-comparison.md)** — combat-feel and content gaps benchmarked against low-poly stylized souls-likes / dungeon-crawlers.

---

## Skimmable summary

### Verdict
- **Genuinely well-architected, C++-first project — above the typical indie/hobby UE codebase.** The engineering fundamentals are right; the gaps are in folder hygiene, a couple of god-classes, and combat *feel/content* rather than code quality.
- **The code is already documented to an unusually high standard** — the doc pass targeted only real gaps (pure-logic `.cpp` files, magic numbers) to avoid noise.

### Architecture — strengths
- Clean single Runtime module; modern include-order pinned in `.Build.cs`/`.Target.cs`; no kitchen-sink `Engine.h` includes.
- Exemplary memory/ownership: `TObjectPtr` throughout (~43 headers), zero raw `new UObject`, no GC-unsafe pointer arrays.
- Proper gameplay framework: GameMode→`TownGameMode` with a `BuildWorld()` hook, GameInstance owning persistent save state, player composed from components.
- 100% pure-C++ UI (`UUserWidget`, no WBP) and code-configured Enhanced Input — fully meets the "C++, never Blueprint" mandate.
- Standout tooling: documented Blender→FBX→`import_meshes.py` pipeline, Interchange presets, commit-count version hook, full Git LFS.

### Architecture — fixes (prioritized)
- **P1 — Flat source folder:** `Source/DungeonCrawler/` holds 121 files in one directory. Subfolder it (Player / Enemies / Dungeon / Items / UI / Core). Biggest structural smell.
- **P2 — God-class:** `FirstPersonCharacter` (1363 lines) bundles combat + fishing + gold + dev cheats + save. Extract into components / a `UCheatManager`. (`DungeonGenerator` is just as big but cleanly decomposed — leave it.)
- **P2 — Asset refs as raw `FString`:** `ItemDatabase`/`MonsterDatabase`/`AffixDatabase` store asset paths as strings, so a renamed/moved asset fails silently. Switch the asset fields to `TSoftObjectPtr`. _(This is exactly today's moved-crab/boss breakage — soft refs would have auto-validated it.)_
- **P3 — Housekeeping:** stray `NewMap.umap`, empty `Content/Collections/`, inconsistent singular/plural Content folder names, a stale `"Sword"` UPROPERTY category, duplicate forward declarations.
- **Noted ceiling:** AI is Tick-based steering on a stock `AAIController` (no Behavior Tree/StateTree) — deliberate, fine for graybox.

### Documentation pass — what was added (11 files, comments only)
- `StatsComponent.h/.cpp`, `CharacterClass.cpp`, `MonsterTypes.cpp`, `SkillTypes.cpp`, `ItemTypes.cpp`, `SkillTreeComponent.cpp`, `EquipmentComponent.cpp`, `HotbarComponent.cpp`, `BossIntroCameraShake.cpp`, `DungeonPlayerController.cpp`.
- Focus: class-role summaries, non-trivial function intent, and gameplay-tuning magic numbers — matched to the existing house style.
- **Refactor candidates flagged for a human:** three near-identical cached-database implementations → a templated helper; duplicated six-field bonus-summing across two components → an `FDerivedBonuses` struct; `SelectSlot`'s redundant-branch control flow; leftover dead Brute/Skitterer note in `MonsterTypes`.

### Gameplay / genre — top improvements (by impact)
- **It's first-person** (`AFirstPersonCharacter`) — affects telegraph readability and which comparables apply.
- **1. Dash i-frames** — single biggest combat-feel fix; dash is currently purely positional (`FirstPersonCharacter.h:317`). ~one-file change. (cf. *Death's Door*, *Enter the Gungeon*)
- **2. Add 2–3 enemy archetypes** (ranged / charger / exploder) — only the Crab exists today; `FMonsterDef` makes this cheap. (cf. *Hades*, *Gungeon*)
- **3. Wire existing affixes/elemental damage into combat** — scaffolded but unused (`ItemTypes.h:78`); connects code you already have.
- **4. Add audio** — none exists in `Source/`; biggest missing pillar (loot pings + attack-telegraph tells, key for FP readability).
- **5. A second defensive verb (block/parry)** — combat is currently dodge-or-eat-it. (cf. *Tunic*, *Sekiro*)
- **Also:** light/heavy or combo melee; enemy windup audio tells; affix re-roll gold sink; per-run "heat" modifiers; death stakes (currently none). See report for quick-wins vs bigger-bets.

---

## Suggested next actions
1. **Quick win, today's-bug-adjacent:** migrate database asset paths to `TSoftObjectPtr` so moves/renames validate instead of breaking silently.
2. **Quick win, combat feel:** add dash i-frames.
3. **Low-effort variety:** author 2–3 new `FMonsterDef` enemy archetypes.
4. **Structural cleanup when convenient:** subfolder `Source/`, and start carving cheats/fishing off `FirstPersonCharacter`.
