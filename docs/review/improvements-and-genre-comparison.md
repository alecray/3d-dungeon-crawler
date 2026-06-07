# Improvements & Genre Comparison — Gameplay/Systems Review

A systems-and-feel review of the 3D dungeon crawler, benchmarked against low-poly / stylized
action-RPGs, dungeon-crawlers, and souls-likes. Scope: **design, systems, and combat feel** — not
art (gameplay is graybox; final meshes drop in later). This intentionally focuses on the
**second-to-second combat angle and content/variety**, which the existing
[`docs/game-loop-analysis.md`](../game-loop-analysis.md) (a strong macro-loop study) covers less.

> **One framing note:** the README calls this a "first-person… dungeon crawler" and the
> code confirms it (`AFirstPersonCharacter`, eye-height camera, FP weapon mesh). The Dark-Souls
> direction is therefore being pursued in **first person**, which changes which comparables apply
> (more *Ziggurat / Witchfire / Dread Templar / Ultra-soulslike-FPS*, less *third-person Souls*) and
> creates specific readability problems called out below.

---

## TOP SUGGESTIONS (ordered by impact)

1. **Add i-frames to the dash (with a tunable window) — the single biggest combat-feel fix.** The dodge is currently *positional only* (no invulnerability; `FirstPersonCharacter.h:317`), which is unusual for a "dodge-based" souls-like and fights against first-person spatial reads.
2. **Build a second-and-third enemy archetype (ranged + charger/exploder).** There is literally **one** enemy type in the database (`Crab`, `MonsterTypes.cpp`). All encounters are "walk up and trade hits." This is the highest-leverage variety win.
3. **Wire the already-scaffolded affixes/elemental damage into actual combat.** `FItemAffix` / `EDamageType` / `FlatBonusDamage` exist but the header itself says "Combat application is wired up later" (`ItemTypes.h:78`). Loot is purely vertical +stats until this lands.
4. **Add audio — there is none.** No `PlaySound`/MetaSound anywhere in `Source/`. Hits, footsteps, telegraph wind-ups, and a rare-loot "ping" are the cheapest, highest-ROI feel multiplier in the genre.
5. **Add a defensive option beyond dodge (block OR parry).** No block/parry/shield exists; combat is dodge-or-eat-it, which thins the moment-to-moment decision space.
6. **Give the basic attack depth: a 2–3 hit combo or a chargeable heavy.** Melee is a single swing on a cooldown (`AttackCooldown`, `MeleeAttack()`); genre peers all have at least light/heavy.
7. **Add enemy attack *audio* telegraphs + a windup-readable tell, especially for FP off-screen threats.** Visual-only telegraphs are a known souls-like failure point and worse in first person where attacks come from outside the FOV.
8. **Roll affix variance onto drops + a town re-roll station (gold sink).** Closes the "deterministic loot" gap and gives gold a hard sink. (Aligns with game-loop-analysis #5.)
9. **Per-run modifiers / "heat" toggles before the portal** — combinatorial variety from systems, not hand-authored content. (Aligns with game-loop-analysis #6.)
10. **Death stakes** — currently death reloads the same dungeon with zero loss (`TODO.md` flags this as undecided). A souls-like with no death cost has no tension.

---

## Current-state assessment (what's actually built)

The project is **systems-rich for a graybox** — well past most solo prototypes. Implemented and
verified in code:

| Area | Status | Evidence |
|---|---|---|
| FP controller, WASD + mouse, jump | Built | `FirstPersonCharacter.h` |
| Dash/dodge (stamina-costed burst) | Built — **no i-frames** | `FirstPersonCharacter.h:317-332`, `.cpp:741-1025` |
| Melee | Built — **single swing, no combo/heavy** | `MeleeAttack()/DoMeleeHit()`, `AttackCooldown` |
| Ranged + Mage styles | Built (stamina/mana gated) | `FireProjectile`, `SpellManaCost` |
| Attributes (STR/INT/DEX/VIT) + level/XP | Built | `StatsComponent.h` |
| Skill tree (3 branches, passives + capstone abilities Q) | Built | `SkillTypes.h` |
| Inventory + equipment + paperdoll + hotbar | Built | `InventoryComponent`, `EquipmentComponent` |
| Loot chests, world pickups, rarity beams, 3D item icons | Built | `LootChest`, `ItemPickup`, `ItemIconSubsystem` |
| Item affixes / elemental damage | **Scaffold only — not in combat** | `ItemTypes.h:77-106` |
| Procedural dungeon (rooms, corridors, loops, room types, traps) | Built | `DungeonGenerator`, `DungeonTrap` |
| Enemy types | **One (Crab)** | `MonsterTypes.cpp` |
| Enemy AI | Chase + melee + backstab only | `MonsterCharacter` (no ranged/charger/etc.) |
| Boss (hermit crab: 3 phases, specials, weak point) | Built — gated behind `bAbilitiesEnabled=false` while anims finish | `BossMonster.h`, README |
| Combat juice (hit-stop, camera kick, damage numbers, vignette, dust, dissolve) | Built & tuned | `TriggerHitStop`, `DamageNumber`, etc. |
| Town hub, shop, portals, bonfire checkpoints | Built | `TownGameMode`, `ShopNPC`, `Bonfire` |
| Minigames (blackjack, fishing) | Built (prototype) | `BlackjackTable`, `FishingHole` |
| Save/profile, stats page, dev menu, pause/settings | Built | `DungeonGameInstance`, `DungeonSaveGame` |
| **Audio** | **None** | no `PlaySound`/`USoundBase` in `Source/` |
| **Block / parry / shield** | **None** | not present anywhere |
| **Death stakes** | **None — reload, no loss** | `TODO.md` "Adjust the death flow (DESIGN DECISION pending)" |

**Verdict:** The *macro* loop (boss-gated unlocks, collection log, gear) and the *production* are
strong. The thin parts are the **inner combat loop** (defensive options, attack depth, dash
i-frames), **enemy variety** (one mob, one AI pattern), and **two missing pillars** (audio,
death stakes). Several systems are scaffolded but not connected (affixes/elements), so they read
as "done" on the feature list but contribute nothing to play yet.

---

## Genre comparison

Comparables chosen for genuine relevance to a low-poly, dodge-based, dungeon-crawling action-RPG.

| Game | What it does well | Where this project stands |
|---|---|---|
| **Death's Door** | Dodge roll has **i-frames, costs nothing, is responsive**; "dreamily precise, fair hitboxes" that let you barely escape; deflectable projectiles add strategy ([Carson Boden](https://carsonboden.com/Library/Design/Comparing-Death's-Door-and-Tunic), [NME](https://www.nme.com/reviews/deaths-door-review-2998793)). | Dash is positional with **no i-frames** and is the *only* defense; no deflect/parry. Closing this is the #1 feel fix. |
| **Tunic** | Stamina-gated combat with **shield block + dodge**, deliberate "commit to your swing" pacing. | Has stamina + dodge but **no block** and a single swing — the decision space is half of Tunic's. |
| **Hades** | Even critics note it leans on **Boons + multiple attack options** (attack/special/cast/dash/summon) to keep encounters varied *despite* limited enemy types ([Rogueliker](https://rogueliker.com/hades-roguepod-litecast/), [ResetEra](https://www.resetera.com/threads/hades-is-a-great-rogue-lite-with-a-disappointing-enemy-and-level-variety.799041/)). | Has 3 styles + a skill tree (good), but only **one enemy** and one AI pattern, so encounters can't vary the way Hades's do. Lean on per-run modifiers + archetypes. |
| **Moonlighter** | Tight **run risk/reward**: die and lose your bag (keep 5 pocketed items); a hold-to-teleport bailout. "I can do this… only to die and lose most of your loot" is the hook ([Switchaboo](https://www.switchaboo.com/moonlighter-review-switch/), [VoxelVoice](https://voxelvoice.com/moonlighter-review/)). | **No death stakes at all** — death reloads with zero loss. The genre's signature tension is entirely unused. |
| **Souls-likes (Dark Souls/Sekiro/Elden Ring)** | Threats telegraphed **through audio** (skeletons assembling, mages charging, trap clicks); Sekiro/ER add **audio cues for unblockables**; readability degrades when telegraphs get "lost in the noise" ([ResetEra accessibility thread](https://www.resetera.com/threads/so-what-accessibility-features-should-from-and-co-implement-in-souls-like-games-read-the-op.561820/)). | **No audio at all**, and first person hides off-screen wind-ups — so telegraph readability is structurally worse here than in third-person Souls. Audio telegraphs are doubly important. |
| **Enter the Gungeon / Crab Champions** | Dodge-roll with i-frames + **distinct enemy archetypes** (chargers, shooters, exploders) that force positioning, not trading. | One mob, one behavior. Archetypes are the cheapest path to "positioning matters." |

**Pattern across all of them:** the games that feel good either (a) give the dodge real
invulnerability, (b) add a second defensive verb (block/parry/deflect), and (c) force
*positioning* via enemy variety. This project currently has none of the three, despite excellent
juice (hit-stop, numbers, beams) sitting on top.

---

## Recommendations — Quick wins

> High impact, low cost. Most touch one file and use systems already present.

### QW1 — Dash i-frames (tunable window)
**What:** During part of `DashDuration`, make the player ignore incoming damage (or take reduced
damage). Add `DashIFrameStart`/`DashIFrameEnd` fractions; check in the damage path.
**Why:** The dodge is sold as the core defensive verb but is positional only
(`FirstPersonCharacter.h:317` literally tunes the dash *distance* to clear the boss zone). In
first person, judging "did I physically leave the disc?" is far harder than third person — i-frames
make dodging feel fair and *readable by timing* instead of by spatial guesswork.
**Comparable:** *Death's Door* / *Enter the Gungeon* — responsive i-frame rolls are the backbone of
"barely escaped" moments.

### QW2 — Light/heavy or a 2–3 hit combo on melee
**What:** Either a chargeable heavy (hold LMB → bigger hit, more stamina, more hit-stop) or a short
combo string that resets on cooldown. The hit-stop/camera-kick scaffolding already varies by hit
(`TriggerHitStop`, `CameraKick(Scale)`).
**Why:** A single swing on a flat cooldown is the shallowest melee in the comparison set. A
heavy/combo creates the commit-vs-poke decision Souls combat is built on, and reuses existing juice.
**Comparable:** *Tunic*/*Dark Souls* light-vs-heavy commitment.

### QW3 — Audio: hits, footsteps, telegraph wind-ups, rarity "ping"
**What:** A small `UGameplayStatics::PlaySound2D/AtLocation` pass at the existing event sites
(melee impact, dash, take-damage, pickup, chest open, boss telegraph brighten, level-up), routed
through one bus tied to the existing master-volume setting.
**Why:** Audio is the cheapest dopamine and the single biggest missing pillar (`TODO.md` calls it
"Biggest non-gameplay gap"). The **rare-loot ping** specifically is most of the loot dopamine
(game-loop-analysis #3). And audio telegraphs are *the* souls-like readability tool
([ResetEra](https://www.resetera.com/threads/so-what-accessibility-features-should-from-and-co-implement-in-souls-like-games-read-the-op.561820/)).
**Comparable:** every souls-like; deaf-accessibility discussions show how load-bearing audio cues are.

### QW4 — Wire affixes + elemental damage into combat
**What:** Roll `FItemAffix` onto drops in `AItemPickup::Configure`/boss loot, apply
`FlatBonusDamage`/`BonusDamageType` in the hit path, and show composed names in tooltips. The
scaffold is already there (`ItemTypes.h:77-106`, `ComposeItemName`).
**Why:** Loot is currently pure +stats. Even basic "+fire damage" affixes make drops feel
different, set up resistances/weaknesses later, and unlock the build-defining-drop direction the
loop analysis rates #1. This is *connecting* existing code, not new architecture.
**Comparable:** *Moonlighter*/*Diablo*-lite itemization.

### QW5 — Enemy attack audio tell + clearer FP windup
**What:** A short wind-up sound + a brief screen-edge directional indicator (or a stronger
mesh/emissive tell) when any enemy commits to an attack, especially from outside the FOV.
**Why:** First person hides the classic third-person windup silhouette. Souls readability already
"gets lost in the noise" in third person; in first person an off-screen crab swing is invisible
until it lands.
**Comparable:** *Sekiro* unblockable kanji + sound; FP soulslikes (*Witchfire*, *Dread Templar*).

### QW6 — Backstab → full stealth/awareness layer (cheap extension)
**What:** Aggro via line-of-sight/noise + an "unaware" bonus on top of the existing 1.8×
positional backstab (`BackstabMultiplier`).
**Why:** The backstab math already exists; adding awareness states turns "trade hits" rooms into
"approach and position" rooms with almost no new combat code.
**Comparable:** *Hades*/*Death's Door* opening-strike incentives.

---

## Recommendations — Bigger bets

> Higher cost, higher ceiling. Each addresses a structural gap.

### BB1 — Enemy archetypes (ranged shooter, charger, exploder, shielded/flanker)
**What:** 3–4 new `FMonsterDef`s with distinct AI: a shooter that kites (reuse `AProjectile`), a
charger that telegraphs a lunge (reuse the boss's `TickCustomChase` lunge), an exploder that rushes
and detonates, a shielded enemy that must be flanked. The data-driven `FMonsterDef` +
`TickCustomChase` hooks make this **architecturally cheap**; only behavior is new.
**Why:** This is the **single biggest variety lever**. With one mob and one pattern, every room is
the same fight; procedural layout can't fix that (game-loop-analysis #8, #11). Even Hades — praised
despite limited variety — relies on shooters/chargers/exploders to force positioning.
**Comparable:** *Enter the Gungeon*, *Hades*, *Crab Champions*.

### BB2 — A real defensive verb: block or parry
**What:** Add a shield/block (hold to reduce damage, costs stamina, can be guard-broken) **or** a
timed parry (deflect projectiles + open a riposte window). Deflectable projectiles specifically add
"a great deal of strategy" in *Death's Door*.
**Why:** Combat is dodge-or-eat-it. A second defensive option doubles the moment-to-moment decision
space and gives the 3 styles + skill tree something defensive to branch into. Pairs naturally with
QW1 (dodge) and BB1 (ranged enemies to deflect against).
**Comparable:** *Tunic* (shield), *Sekiro*/*Death's Door* (deflect/parry).

### BB3 — Death stakes tied to reward
**What:** Drop accumulated gold / a re-roll currency on death, retrievable by reaching the spot
alive (Souls bloodstain), or a Moonlighter-style "lose the bag, keep N pocketed items" with a
hold-to-bail teleport. Resolve the `TODO.md` "death flow (DESIGN DECISION pending)" toward *some*
cost.
**Why:** A souls-like whose death does nothing has no tension. Risk/reward is the genre's signature
engagement source and is currently completely unused (game-loop-analysis #7).
**Comparable:** *Moonlighter* (bag loss + bailout), *Dark Souls* (bloodstain retrieval).

### BB4 — Per-run modifiers / "heat" before the portal
**What:** A portal screen offering threats-and-boons toggles ("enemies explode on death," "no
healing but +50% loot," "elite-dense, double rarity," "low visibility"). Each crank raises
difficulty *and* reward together. Doubles as the scalable-difficulty tier system already in the
backlog.
**Why:** Combinatorial, systemic variety from a handful of toggles — far cheaper than authoring new
dungeon types, and it turns greed into a playstyle. Strongest content-per-effort multiplier
available (game-loop-analysis #6).
**Comparable:** *Hades* Pact of Punishment / *D3* Greater Rift affixes.

### BB5 — Item re-roll / upgrade station (the gold sink + chase multiplier)
**What:** Town station: spend gold + boss materials to re-roll an affix or bump item tier. Drop boss
uniques with **variable** affix rolls (scaffold exists). Turns each duplicate boss kill into a
"better-rolled version" chase and gives gold its first hard sink.
**Why:** Connects affixes (QW4) to a long-tail loop and fixes the flat economy (no repeatable gold
sink today; shop is one-and-done). (game-loop-analysis #5.)
**Comparable:** *Last Epoch* / *D4* tempering, scaled down.

### BB6 — Build-defining boss drops (horizontal loot)
**What:** Each signature unique *changes how you play* (dodge → short blink, fireball chains, heavy
attack consumes a resource for AoE) rather than just adding numbers.
**Why:** Multiplies replay value off the 3 styles + tree at low authoring cost, and is the
highest-ROI item in the existing loop analysis (#1). Worth restating here because it depends on
QW4/BB5 landing first.
**Comparable:** *Hades* Boons, *Dead Cells* mutations, *Binding of Isaac* items.

---

## Lower-priority but worth noting

- **Lock-on / soft-target assist (FP):** even a light "snap crosshair toward nearest enemy on
  attack" helps FP melee land; full lock-on is a bigger lift but standard in third-person souls.
- **Colorblind-safe rarity colors + telegraph shapes** (not just red): the boss telegraph is a red
  disc; readability in a dark dungeon benefits from shape/animation cues, not color alone (already
  in `TODO.md` accessibility).
- **Mob spawn VFX/poof parity:** boss has a spawn-in; regular mobs pop in (noted in `TODO.md`).
- **Minigame integration:** fishing/blackjack are isolated novelties; tie catches to crafting
  reagents and blackjack to a controlled gold sink so they re-enter the loop (game-loop-analysis #10).
- **Remove `bAbilitiesEnabled` gate** once boss anims land so the full 3-phase fight is the default
  (tracked in `TODO.md`); right now the shipped boss is its simplest form.

---

## The smallest high-impact set

If only five things ship next, do these — they fix the inner combat loop and the two missing
pillars, mostly by *connecting* existing code:

1. **QW1 dash i-frames** — makes the core defensive verb fair (one file).
2. **BB1 two enemy archetypes** — kills the "one fight forever" problem (data-driven, cheap).
3. **QW3 audio pass** — biggest feel multiplier; unlocks audio telegraphs.
4. **QW4 wire affixes/elements** — turns loot from +stats into variety (scaffold exists).
5. **BB3 death stakes** — gives the souls-like its missing tension.

---

## Sources

- [Comparing Death's Door and Tunic — Carson Boden](https://carsonboden.com/Library/Design/Comparing-Death's-Door-and-Tunic)
- [Death's Door review — NME](https://www.nme.com/reviews/deaths-door-review-2998793)
- [Hades enemy/level variety discussion — ResetEra](https://www.resetera.com/threads/hades-is-a-great-rogue-lite-with-a-disappointing-enemy-and-level-variety.799041/)
- [Hades roguelite review — Rogueliker](https://rogueliker.com/hades-roguepod-litecast/)
- [Moonlighter review — Switchaboo](https://www.switchaboo.com/moonlighter-review-switch/)
- [Moonlighter review — VoxelVoice](https://voxelvoice.com/moonlighter-review/)
- [Souls-like accessibility / audio telegraphs — ResetEra](https://www.resetera.com/threads/so-what-accessibility-features-should-from-and-co-implement-in-souls-like-games-read-the-op.561820/)
- Internal: [`docs/game-loop-analysis.md`](../game-loop-analysis.md) (macro-loop study this report complements)
