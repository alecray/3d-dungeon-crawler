# Game Loop Analysis & Improvement Plan

A design review of the game loop drafted in `ideas.md`, compared against what makes
RPG/ARPG/MMO loops engaging and addictive. Synthesized from three independent research
passes (core-loop reward psychology · long-term progression & endgame retention · variety
& motivation frameworks). Tailored to a **solo dev** building a single-player, first-person,
Dark-Souls-flavored UE5 dungeon crawler.

---

## Your loop (as written)

> start/continue → enter portal → kill monsters for xp/gold/items → find & kill boss
> (each boss has a unique drop) → gear up + level skills → felling a boss unlocks 1+ other
> dungeon types/maps (with bosses) **and** the next upgraded variant of that boss →
> **endgame:** complete the collection log, slay all bosses.

## One-line verdict

**You've drafted ~90% of a great ARPG.** The macro-structure is genuinely strong — your
boss-gated unlock tree is *better* than the "menu of difficulty checkboxes" most ARPGs ship,
and you independently arrived at the genre's #1 retention primitive (escalating boss variants).
The two real problems are both fixable with systems you already have scaffolded:

1. The written loop is **all outer loop** — it specifies *structure* but not the two **inner
   loops** (combat/loot *juice* and the **variable-ratio loot lottery**) where second-to-second
   addictiveness actually comes from.
2. The endgame is defined as a **finish line** ("complete the log, slay all bosses") where the
   genre defines a **treadmill**. At 100%, every reason to play disappears at once.

---

## The three nested loops (and where yours is thin)

Addictiveness lives in three loops nested by timescale. Your design specifies the outermost; the
inner two are where the dopamine is.

| Loop | Timescale | What drives it | Your loop |
|------|-----------|----------------|-----------|
| **A — Combat juice** | seconds | damage numbers, crit hitstop, hit/kill feedback | **unspecified** |
| **B — Loot lottery** | per run | variable-ratio drops, rarity explosions, near-misses, re-rolls | **partially** (deterministic boss drops only) |
| **C — Meta-structure** | weeks | unlock tree, power growth, collection, completion | **strong** ✅ |

---

## What's already strong (keep these)

- **Boss-gated unlock tree** — a legible, metroidvania-flavored progression spine the player
  *reveals* as a content map. More motivating than a difficulty dropdown.
- **"Upgraded boss variant on kill"** — this is a *latent infinite-scaling ladder* (D3 Greater
  Rifts / Last Epoch Corruption). It's the single most valuable line in your design; it just
  isn't infinite *yet* (see Suggestion #2).
- **Unique boss drops** — natural chase items that feed the collection log.
- **Collection log** — a proven completionist driver with built-in milestone hooks.
- **Raw materials already in the codebase**: 3 combat styles, the skill tree, rarity-colored
  loot with **scaffolded affixes/ilvl** (`FItemAffix`/`ComposeItemBonuses`), a gold economy +
  shop, and the procedural dungeon generator. Most suggestions below just *connect* these.

---

## Gap analysis (consolidated)

1. **No loot/combat *feedback engine*.** "Kill monsters for items" says nothing about damage
   numbers, crits, hit/kill feedback, rarity beams/sounds, or an identify/reveal beat — i.e. the
   actual dopamine source is absent from the loop.
2. **Rewards are deterministic, not a variable-ratio schedule.** Boss uniques are one-and-done;
   once you have the drop the boss has no pull, and trash mobs aren't a continuous "*might* drop
   something amazing" gamble. Variable-ratio reinforcement (the slot-machine schedule) is the
   stickiest reward structure known and it's currently missing from the minute-to-minute grind.
3. **The endgame is finite — and the design names its own end.** Completionism over a fixed set
   has a hard wall: at 100% the motivation evaporates. Great *spine*, poor *terminus*. There is
   no post-100% loop (no infinite scaling, personal-best, re-roll, or fresh-start reason).
4. **Itemization is vertical, not horizontal.** "Gear up" reads as bigger numbers. Without
   **build-defining** items that *change how you play*, loot collapses into +stats and loses its
   long-tail pull — and you forfeit the autonomy that makes the 3 styles + tree replayable.
5. **Goals collapse to one time horizon.** Everything between "kill this boss" and "finish
   everything" is missing — no session goals, quests, dailies, or milestone payouts. Players need
   a *next small thing* every session.
6. **No safety-net / re-roll / crafting currency.** Rolled affixes without a way to push toward
   wanted stats become pure frustration RNG; and gold has no repeatable hard **sink**, so the
   economy goes flat.
7. **The Souls risk/reward hook is unused.** It's a Souls-flavored game with no death stakes tied
   to reward (no drop-currency-on-death-retrieve-it run tension) and no intra-run risk/reward
   choices — the genre's signature engagement source, left on the table.
8. **Variety leans entirely on hand-authored content.** Procedural layout alone gets pattern-
   matched fast; there's no *systemic* per-run variety, so every new dungeon type is expensive
   art that's "solved" once seen.
9. **The unlock graph risks being linear** — losing route autonomy and the pressure-valve that
   lets a stuck player go do something else.
10. **Town minigames (fishing/blackjack) are throwaways** — novelties with no loop integration,
    so they get played once and abandoned.
11. **Upgraded boss variants risk being "+HP" only.** Monster Hunter's lesson: variable *behavior*
    is what makes re-killing the same boss fun; a fixed list of HP-bumped tiers still ends.

---

## Suggestions, ranked by engagement-gained ÷ solo-dev-cost

> The top four convert content you already have into hundreds of hours; none require
> MMO-scale authoring.

### #1 — Build-defining boss drops (horizontal, not vertical) — *highest ROI*
Make each signature unique **change how you play**, not just add power: a drop that turns your
dodge into a short blink, makes fireball chain, lets a heavy attack consume a resource for an AoE,
makes crits apply a DoT, etc. This converts loot from vertical → horizontal, branches your 3
styles + skill tree into *many* builds, and is the cheapest way to multiply replay value. Hunt for
"aha-synergy" between drops, tree nodes, and styles. (Uses `FItemAffix`/the equip system.)

### #2 — Turn "upgraded boss variants" into an INFINITE ascending ladder — *the endgame fix*
After a boss's hand-authored tiers (base → Champion → Nightmare), every further kill spawns it at
**+1 affliction tier**: monster HP/damage scale a fixed % per tier, and rewards (ilvl, gold, drop
rarity, XP) scale alongside. Track **highest tier cleared per boss** + one overall **"deepest tier"
personal best**. This single change converts your named *finish line* into a *treadmill* and gives
the Souls combat a reason to stay lethal forever. Implementation: one stat multiplier + one reward
multiplier driven by tier number. **Do this one.**

### #3 — Combat + loot juice — *cheapest, biggest second-to-second payoff*
- **Floating damage numbers** (scale with magnitude; crits get a distinct color, bigger size,
  ~0.12 s hitstop, punchier SFX). You already have a hit-stop system (`TriggerHitStop`) and a hit
  marker — extend them. Reserve the loudest feedback for crits/rare kills so it stays meaningful.
- **Loot drop juice**: rarity-colored beam/glow + a *tiered drop sound* (the legendary "ping" must
  be unmistakable and rare) + a brief **reveal/identify beat**. You already compute `RarityColor`
  and emit loot beams on world pickups — lean in. This single effect is most of the dopamine.

### #4 — Make the variable-ratio loot lottery real
Every kill should be a lottery ticket: trash mobs roll on a loot table where rare/build-defining
items are *possible but improbable*, **biased toward the current build**, with higher rarity
statistically ≥ lower. Surface **near-misses** (show an affix's roll % within its range / what it
*could* have rolled) so "almost perfect" reads clearly and pulls the player back. The gambling
layer must run *continuously*, not only at bosses.

### #5 — Item re-roll / crafting layer + currency *(chase multiplier AND the gold sink)*
Drop boss uniques with **variable** affix rolls (the scaffolding exists), then add a town
**re-roll/upgrade station** (spend gold + boss materials to re-roll an affix or bump ilvl tier).
This does three things at once: makes every duplicate boss kill meaningful ("best-rolled version"
chase), creates the hard **gold sink** the economy needs, and extends each unique from a one-time
grab into a hundreds-of-hours refinement. (D4 Tempering / Last Epoch, scaled down.)

### #6 — Per-run dungeon modifiers/affixes — *biggest variety multiplier, content-cheap*
Before a portal, let the player pick/accept **run modifiers** — threats *and* boons: "enemies
explode on death," "no healing but +50% loot," "elite-dense, double rarity," "fog/low-vis." A
handful of toggles yields combinatorial variety, doubles as your scalable-difficulty / "Heat"
system, and turns greed into a playstyle (scarier modifier → better loot/log entries).

### #7 — Layer goals across time horizons
Add explicit mid-tier goals between "kill boss" and "100% the log":
- **Session goals / quests:** "clear 3 dungeons with a fire build," "kill a boss without healing,"
  "catch a rare fish."
- **Dailies/weeklies:** a rotating modifier + objective for a bonus — a reason to log in *today*.
- **Collection-log milestones:** payouts at 25/50/75/100% (cosmetics, titles, a hub trophy room),
  and **infinite-tail entries** the log never fully completes ("deepest tier per boss," kill-count
  tiers). Reframe 100% as **"Prestige unlocked,"** not "game over." (Mostly data/UI.)

### #8 — Wire the Souls risk/reward into the *reward* loop
- **Death stakes on the run:** drop accumulated gold / re-roll currency on death, retrievable only
  if you reach the spot alive — the "bank now or push one more room" tension is the Souls signature.
- **Intra-run choices:** cursed chests (power now, debuff for the run), shrines ("sacrifice 30% HP
  for a rare affix"), optional mini-boss rooms gating better loot. Turns difficulty into
  self-authored buildcraft.

### #9 — Branch the boss-unlock graph & make upgraded bosses behave differently
Have each boss open **2+ divergent paths** ("kill any 2 of 3 to unlock the next tier") for route
autonomy and a pressure valve. And make upgraded variants change **moves/patterns**, not just HP
(MH lesson) — at minimum add a new attack per tier.

### #10 — Integrate the town minigames as real side-loops
- **Fishing** → catch crafting reagents/cosmetics that feed #5 (a rare fish = ingredient for a
  build-defining mod), with its own collection-log section.
- **Blackjack** → a controlled gold sink: gamble dungeon gold for shop currency or a chance at a
  re-roll token.
- Both also give a deliberate **tension-release** beat between punishing combat. Near-free since
  the systems already exist.

### Bonus — open-ended tracks & sinks (low effort, balance-neutral)
- **Paragon track**: convert overflow XP past the cap into an infinite secondary stat/token track —
  keeps the kill→XP loop alive forever and feeds #2/#4.
- **Cosmetic/trophy vendor**: dyes, weapon skins, hub decorations bought with gold — an "infinitely
  scalable," balance-neutral hard sink for surplus late-game gold (also feeds the log).
- **Lightweight "Ascension" runs**: an optional fresh-start with a rotating global modifier + its
  own local leaderboard — the solo-dev analog of seasons; combats build-calcification.

---

## The smallest set of changes for the biggest impact

If you only do four things: **#1 build-defining drops**, **#2 infinite boss ladder**, **#3
combat/loot juice**, **#5 re-roll station (+gold sink)**. Together they fix the two core problems
(thin inner loops + finite endgame) using systems you've already scaffolded, and they make the
boss-gated tree you invented genuinely best-in-class.

## Guardrails

- Favor **horizontal** unlocks (new toys/rule-changers) over **vertical** (+stats) — vertical
  power-creeps your own combat into boredom.
- Keep meta-progression making runs *different*, not *trivially easier*.
- Never grind to pad playtime — every collection target should be reachable through the fun core
  loop, and encounters varied enough that repetition stays engaging (fair, learnable Souls
  patterns = competence payoff, not frustration).

---

## Sources

**Core loop / reward psychology:** [The ARPG Loop Players Never Get Tired Of (Fextralife)](https://fextralife.com/the-arpg-loop-players-never-get-tired-of/) ·
[ARPG Itemization (Marc Léo Séguin)](https://marcleoseguin.com/2026/04/08/arpg-itemization/) ·
[PoE2 vs D4 loot dopamine (PC Gamer)](https://www.pcgamer.com/games/rpg/path-of-exile-2-is-great-but-it-cant-match-the-dopamine-hit-of-a-good-diablo-4-loot-drop/) ·
[Juice in Game Design (Blood Moon)](https://www.bloodmooninteractive.com/articles/juice.html) ·
[The "Juice" Problem (Wayline)](https://www.wayline.io/blog/the-juice-problem-how-exaggerated-feedback-is-harming-game-design) ·
[Risk vs Reward: the Souls-like twist (Gameopedia)](https://gameopedia.com/blogs/risk-vs-reward-the-souls-like-s-twist-on-multiplayer)

**Progression / endgame retention:** [D3 Greater Rifts (Maxroll)](https://maxroll.gg/d3/resources/greater-rifts) ·
[Last Epoch Empowered Monoliths (Maxroll)](https://maxroll.gg/last-epoch/monolith/empowered-guide) ·
[D4 Torment/Pit difficulty (Fextralife)](https://diablo4.wiki.fextralife.com/Difficulty+Modes) ·
[PoE Atlas Passive Tree (Maxroll)](https://maxroll.gg/poe/getting-started/atlas-passive-tree) ·
[Lord of Hatred = loot chase again (Icy Veins)](https://www.icy-veins.com/d4/news/lord-of-hatred-turns-every-unique-drop-into-a-real-loot-chase-again/) ·
[Gold sinks (Wikipedia)](https://en.wikipedia.org/wiki/Gold_sink) ·
[Fun, Grief, Completionism (Game Developer)](https://www.gamedeveloper.com/design/fun-grief-completionism-in-games)

**Variety / motivation frameworks:** [Compulsion loop (Wikipedia)](https://en.wikipedia.org/wiki/Compulsion_loop) ·
[Self-Determination Theory for games (Game Developer)](https://www.gamedeveloper.com/design/a-quick-breakdown-of-self-determination-theory) ·
[Core loops in gameplay (Game Design Skills)](https://gamedesignskills.com/game-design/core-loops-in-gameplay/) ·
[Roguelite progression systems (GameRant)](https://gamerant.com/roguelite-games-with-best-progression-systems/) ·
[Warframe vs Monster Hunter: grinding done right](https://forums.warframe.com/topic/419457-warframe-vs-monster-hunter-how-grinding-is-done-right/) ·
[Flow Theory in Game Design (Blood Moon)](https://www.bloodmooninteractive.com/articles/flow-theory.html)
