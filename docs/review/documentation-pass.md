# In-code documentation pass

A comment-only pass over `Source/DungeonCrawler/`. **No code behavior was changed** — only comments
were added (and only to source files). The codebase already has an unusually high documentation bar:
headers carry thorough `/** */` class/function summaries, and most `.cpp` files already explain their
tricky spots inline. The work below targets the genuine gaps — pure-logic `.cpp` files that lacked
function-level summaries, and a handful of undocumented magic numbers / tuning constants.

The existing house style was matched throughout: `/** */` for API doc on headers, plain `//` for
"why"/intent comments above functions and beside non-obvious numbers, no comments that merely restate
the code.

## Summary (what was documented)

- **StatsComponent.h / .cpp** — documented the derived-stat formulas (base + per-attribute scaling),
  the linear XP curve, and the per-level rewards (1 skill point, 3 attribute points); added function
  summaries to `AddXP`, `LevelUp`, `AddAttributePoint`, `LoadFrom`.
- **CharacterClass.cpp** — explained the class design philosophy behind the three starting attribute
  spreads and labeled each archetype's intent (DEX glass-cannon / INT caster / STR bruiser).
- **MonsterTypes.cpp** — documented the cached-static database pattern, the `IndexById` O(1) lookup,
  and the seeded weighted random-roll.
- **SkillTypes.cpp** — added an overview of the 3-branch / 5-tier tree shape and the role of the
  `Make` / `MakeMod` / `MakeAbility` factory lambdas.
- **ItemTypes.cpp** — documented the intentional multi-pass structure of `BuildItems()` (create, then
  patch optional data by id).
- **SkillTreeComponent.cpp** — explained `RecomputeBonuses` (full recompute on change, separate
  bonus channel from gear, per-rank scaling).
- **EquipmentComponent.cpp** — explained `RecomputeBonuses` (overwrite-not-add, separate channel from
  the skill tree).
- **HotbarComponent.cpp** — documented why `SelectSlot` re-broadcasts even when re-selecting the active
  slot (re-equip / holster behavior).
- **BossIntroCameraShake.cpp** — explained the mismatched per-axis oscillation frequencies and the
  amplitude units in the shake update.
- **DungeonPlayerController.cpp** — added function summaries for the two central, non-obvious flows:
  `UpdateInputMode` (the single input-mode arbiter) and `FadeToBlackAndTravel` (deferred-travel latch).

## Details per file

### StatsComponent.h / StatsComponent.cpp
The derived-stat getters (`GetMaxHealth`, damage mults, etc.) are dense one-liners packing a flat base,
a per-attribute coefficient, and two separate bonus channels. Added a block comment explaining the
common shape (and why the mults read as `1.0 + 0.05*attr`). Documented `XPToNextLevel`'s linear curve
(`100 + (level-1)*75`) and, in the `.cpp`, the previously-uncommented level-up rewards. `AddXP` now
notes it can roll over multiple levels in one call. `LoadFrom` notes the per-field clamping is there to
survive a corrupt/edited save.

### CharacterClass.cpp
The three `case` blocks set raw attribute numbers with no rationale. Added a header comment describing
the design rule (invest in the weapon's scaling attribute + survivability, dump the off-stat, keep the
totals roughly equal) and a one-line intent label on each class.

### MonsterTypes.cpp
Already had good inline comments on the Crab def. Added namespace- and function-level comments covering
the build-once/cache-in-statics idiom, the `IndexById` map (why it exists — O(1) Get/Contains), and the
weighted `RollRandomType` (deterministic via the seeded stream; currently always returns the Crab since
it's the only weighted type).

### SkillTypes.cpp
Well-organized with section comments and self-documenting node descriptions. Added a namespace-level
overview of the tree's uniform shape across the three branches (tiers 0–4: damage / survivability /
capstone / pick-one modifier / active ability) and the prereq-only gating, plus a note on what the three
factory lambdas exist to do.

### ItemTypes.cpp
The lookup helpers and rarity weights were already commented. Added a comment on the non-obvious
multi-pass structure of `BuildItems()` (items created first, then optional equip/desc/icon data patched
in by id) since a reader could otherwise wonder why fields are set in separate later passes.

### SkillTreeComponent.cpp / EquipmentComponent.cpp
Both headers are thorough. The `.cpp` `RecomputeBonuses` in each is the one spot worth a "why" note:
each recomputes the full total from scratch on every change (no drift), and each writes to a *separate*
bonus channel on `UStatsComponent` (`Bonus*` for skills, `EquipBonus*` for gear) so the two sources
stack instead of clobbering. Documented both.

### HotbarComponent.cpp
`SelectSlot` has an `else if` branch that re-broadcasts when you re-select the already-active slot. There
was a one-line inline note; added a function-level comment making the intent explicit (the listener
treats the broadcast as "re-equip", so the same hotkey re-draws/holsters the weapon).

### BossIntroCameraShake.cpp
The shake update is a wall of `FMath::Sin(... * frequency) * amplitude` terms. Added a comment
explaining that the frequencies (16–22 Hz) are deliberately mismatched and phase-offset to avoid an
obvious repeating wobble, and that amplitudes are cm (location) / degrees (rotation) with vertical
emphasized for impact.

### DungeonPlayerController.cpp
Most spots were already inline-commented. Added function-level comments to the two flows whose
correctness depends on subtle coordination:
- `UpdateInputMode` — the single arbiter every panel routes through, which is *why* closing one of two
  stacked panels keeps the cursor on.
- `FadeToBlackAndTravel` — the fade-then-`OpenLevel`-on-timer pattern, with `PendingTravelMap` doubling
  as the in-progress latch that swallows a double-trigger during the fade.

## Spots that may warrant a human's attention (possible refactor candidates)

These are not bugs — just observations a maintainer might want to look at:

- **Repeated cached-database boilerplate.** `MonsterDatabase`, `SkillDatabase`, and `ItemDatabase` each
  hand-roll the identical `Build…()` + cached `…()` accessor + `IndexById()` + `Get`/`Contains`/`All`
  pattern. This is fine and clear, but it's three near-verbatim copies — a small templated
  `TStaticDatabase<T>` helper would remove the duplication if the project keeps adding databases.
- **Duplicated bonus-summing logic.** `SkillTreeComponent::RecomputeBonuses` and
  `EquipmentComponent::RecomputeBonuses` both iterate a source and fan the same six fields
  (MaxHealth/MaxMana/MaxStamina + 3 damage mults) onto `UStatsComponent`. The two parallel
  `Bonus*` / `EquipBonus*` field sets on `UStatsComponent` are also nearly identical. If a seventh
  derived stat is ever added it has to be threaded through several places by hand; an `FDerivedBonuses`
  struct with `operator+=` (and one per source) would centralize it.
- **`HotbarComponent::SelectSlot` control flow.** The `if (… && Index != ActiveIndex)` /
  `else if (…)` split does the same broadcast in both branches; it reads slightly awkwardly. It could be
  flattened to a single validity check that updates `ActiveIndex` then always broadcasts — behavior would
  be identical (now documented so the intent is clear either way).
- **`MonsterTypes` dead definitions.** The file notes that Brute/Skitterer were removed and only the
  Crab spawns. Worth a decision at some point: re-add them with weights, or delete the note — right now
  the weighted roll machinery is exercised by a single entry.
