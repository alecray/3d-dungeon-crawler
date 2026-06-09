# Consolidated Test Plan

Untested / unchecked work is up top (**TO TEST**); verified work is at the bottom (**PASSED**).
**(NEW)** = added this revision, never run. Items the user flagged with `?` were moved to `TODO.md`.

**Setup**
- Rebuild the editor target, open the project, **Play**.
- The game boots to **L_Town**; the start menu appears first.
- To test a fresh profile / first-time cinematics, delete `Saved/SaveGames/DungeonProfile.sav` first.
- Kill crabs for XP / skill points. Enter the dungeon via the town portal.
- Tip: **Dev Menu** (Esc → Dev Menu) has **Teleport to Boss**, God Mode, Reveal Map, Teleport Home for fast testing.

---

# TO TEST

## 0. This session's changes (NEW — test these first)

**Boss tiers + multi-life phasing + ground slam:**
- [ ] Boss **HP and damage scale with the selected tier** (0-3); tier 0 = baseline.
- [ ] **Multi-life phasing**: bringing the boss to 0 HP **heals it to full and advances a phase** instead of killing it, until its last life — then it dies. **Tier 0 has exactly 2 phases** (no phase 3).
- [ ] **Star indicator** beside the boss health bar shows phases/lives left and counts down as you phase it (★★ → ★ → dead); hidden when there's only 1 phase.
- [ ] **Phase 2 ground slam** — a large **telegraphed AOE** disc; you take damage only if **grounded** at impact, so **dodge by jumping** (airborne = safe). A second slam can't stack during a wind-up.

**Portal map/tier select menu:**
- [ ] Interacting with the town portal **opens a map + tier (0-3) menu** (mouse cursor appears) instead of travelling immediately; the destination name shows the real map; pick a tier, hit **Go** → fade + travel into the dungeon at that tier. **Cancel** (or **E**) closes it without travelling.
- [ ] The tier picked actually applies to that run's boss.

**Town day/night cycle:**
- [ ] In town the **sun arcs** over ~2 min (tunable); sky + fog shift **sunrise → noon → sunset → night**; a **moon + stars** appear at night and the town stays **readable** (not pitch black).
- [ ] **Dawn is clean** — no staticy/shimmering sky (stars fully fade before dawn).
- [ ] The **dungeon** is unaffected (separate lighting).

**Death system + Death Scroll:**
- [ ] Dying sends you **back to L_Town** (not a dungeon reload) and **deducts gold** (cost scales softly with level); the inventory screen shows the current **"Death cost: N g"** line.
- [ ] With a **Death Scroll** in your inventory (buy from the shop, ~200 g), dying instead **consumes the scroll** — revive at **half health/mana/stamina**, no gold toll, no town trip. The **Stats screen** shows **Death Scrolls Used**.

**Loot:**
- [ ] **Fishing-only catches** (Minnow / Sardine / Trout / Bass / Pike / Golden Carp / Old Boot) **no longer drop from chests or the boss** — they come only from fishing.

## 1. Carried over (still untested)

**Dash i-frames:**
- [ ] Dashing (Shift) grants a brief **invulnerability window** (~first 0.15s of the 0.2s dash) — time it into an attack to roll *through* the hit; God Mode unaffected by dashing.

**Refactor regression sweep (behaviour should be UNCHANGED)** — player split into Fishing/Combat components, gold→GameInstance, cheats→controller, source subfoldered, enemy assets relocated:
- [ ] Combat — melee + wall-deflect, crossbow (stamina), mage bolt (mana, mid-cast release), Q abilities (Whirlwind/Volley/Nova); hit-stop + camera kick + hit-marker.
- [ ] Weapon swap sets style + held mesh; fishing flow (cast/idle/tension/reel + rod stow/restore); gold (shop/blackjack/give) shares one persisted value; dev cheats (No Clip/God/Kill); crab + boss still animate.

**Blackjack overhaul (town):**
- [ ] Placed in L_Town, flat + blocks you, `[E] Play Blackjack`; real card-face images, dealer hole card face-down till resolve; pull-to-seat camera; your row nearest, dealer behind, newest card on the left; left panel Gold/Bet/buttons, 3D "Dealer N / You N" labels, bottom-center status.

**Other carried-over:**
- [ ] **Boss feel** — forward attack telegraph matches the hit; boss self-glow reads in the dark; melee hit lands on the swing's frame 20.
- [ ] **Bonfire** (Rest rooms): `[E] Rest` fully heals + refills + saves; reusable.
- [ ] **Mage weapon** — Wand/Staff casts mana spell bolts (Mage style).
- [ ] **Backstab** — behind a regular enemy = extra damage (orange crit).
- [ ] **Starting class** (fresh save) — Warrior/Ranger/Mage seeds stats + weapon; returning save shows Continue.
- [ ] **Damage numbers** at the impact point (linger ~2.2s, rise); red **"-N"** popup over the HP bar on damage.
- [ ] **Dash distance** consistent (no over-fling); **equipping Max-Health gear** doesn't flash the screen red.
- [ ] **Level-up burst**, **enemy knockback**, and a **feel check** (juice not nauseating / overlong).

---

# PASSED (verified)

## Boss — mesh / room / intro / sequence
- [x] Boss is the **hermit-crab skeletal mesh**, boss-sized, upright; **idle** standing, **walk** while chasing; **stands on the floor** (not sunk).
- [x] Boss room is a **double-height arena** (no clipping between wall courses), **banner each side of every doorway** (lying flat), **no traps/chest** inside.
- [x] **Intro camera frames the boss** (centered, scaled to fit), and plays **every encounter incl. replays**: input locks, boss spawns opposite you with the spawn-in VFX + screen shake, then control returns.
- [x] No "Phase 9" / multiple-boss leftovers when the dungeon regenerates.
- [x] A clean boss hit is **brutal (~75% HP)**; it closes fast. On death it **drops several rarity-beamed items** at the kill spot and the **return portal** appears in a back corner. Spawn-in code VFX = ground flare + energy pillar + shard swirl + debris ring.
- [x] Boss **loot drops on the floor** (not the ceiling); **banners lie flat**.

## Combat / feel
- [x] **Dash (Shift)**: quick **stamina-costed dash** in your movement/facing direction (not a sprint); cooldown-gated; no stamina → **stamina bar flashes red** and no dash.
- [x] **Insufficient-resource**: swinging/firing with no stamina (or casting with no mana) **flashes the matching HUD bar red**.
- [x] **Player melee lands partway** through the swing; **hit sparks are a dark blood spurt** (not a bright pop).
- [x] **Damage numbers** are big/bright and pop on hit. *(Reworked this session — re-test in TO TEST.)*
- [x] **Enemy death** sinks + shrinks (dissolve). **Traps glow red** before firing. **Wall torches** sit lower, light from the flame tip.
- [x] **Pickup** throws a rarity-colored sparkle; **chest open** pops a rarity burst (brighter for Rare+).

## VFX / "alive" pass
- [x] Impact sparks, ambient drifting dust motes, **screen damage flash** on hit, **low-HP** chromatic aberration + desaturation, **dash FOV** widen. **FPS holds** with all of it on.

## Scenery
- [x] Furniture **lines the walls** flush, facing into the room; **corner stacks** + **dining sets** (table ringed by stools); **coffins flat** along the wall; **no scenery on traps** or inside other scenery; corridor props don't block the path. *(Note: prop clusters could still be improved.)*

## Dev menu / shop / inventory
- [x] **Esc → Dev Menu**: No Clip / God Mode / Reveal Map / Teleport to Boss / Kill Player / Teleport Home — all work as labelled.
- [x] Inventory shows **"Gold: N"**; **shop and inventory are mutually exclusive** (never overlap).

## Traps / room types
- [x] **Spike floors / pressure plates / dart shooters** work; not in the spawn room or boss room.
- [x] Room types flagged by a **colored marker light**: gold = Treasure, red = Ambush, green = Rest, purple = Elite.

## Skill tree / numbers / stamina / HUD
- [x] **K** opens Melee/Ranged/Mage columns + Points; **one clean line per node** with a hover tooltip; allocate/lock/multi-rank/capstones persist across relaunch. *(Note: want to redo all nodes with custom ones later.)*
- [x] Floating damage numbers face the camera; **backstab / weak-point** are bigger + orange. *(Note: improved this session.)*
- [x] A melee swing **drains stamina**; out of stamina = no swing.
- [x] **FPS counter** top-right under the minimap, color-coded; pause menu reads cleanly (not squished).

## Interaction / town / regression
- [x] `[E] Open` / `[E] Pick up` / `[E] Close` prompts; dropped items show their mesh.
- [x] Crabs **path around walls/corners** then close in and attack, no stutter.
- [x] Starts in **L_Town**; portal (`[E] Enter`) loads a dungeon with a fade; ShopNPC buys/sells; return portal flush on a wall; **gold/inventory/equipment/skills persist** across transitions.
- [x] HUD bars + level correct; styles switch with weapon; chests loot; dungeon generates; inventory/hotbar/collection-log/XP/gold persist across relaunch; no bright Lumen band on wall tops.

## Prototypes
- [x] **Fishing (town)**: `[E]` casts (red bobber); after a few seconds it **shakes** → **REEL IN!** — press `[E]` within ~1.3s to land a **random fish**; miss = "it got away".
- [x] **Blackjack (first-pass prototype)**: `[E]` opens a control bar; cards on the felt; bet gold vs the dealer (3:2 blackjack). *(Overhauled this session — re-test in TO TEST.)*

## Earlier-verified
- [x] **Start screen** (menu on launch, once per session); **scene transitions** (black fades + fade-in); **controls in settings** (scrollable list); **boss health bar** (top-center, drains, removed on death); **minimap fog of war** (reveals as you walk, boss room red, no crash).

---

## Known / expected rough edges (not bugs)
- The boss now **tiers + phases for real** (the `bAbilitiesEnabled` anim-test gate was removed). At tier 0 only the **ground slam** special is enabled (phase 2); the others (projectile volley, summon adds, bubble pools, enrage, shell-retreat) are **reserved for tier 1+** (deferred). Special-attack **animation clips** aren't authored yet, so those abilities currently play no clip — the gameplay still fires.
- The **weapon-rack** prop spawns tilted/sunk (asset pivot fix is on the art TODO).
- **Navmesh hand-off** is untested — there are no walls in the boss arena yet to break line of sight.
- Still graybox in places (doors / portal / traps / bubbles / room-markers / bonfire). The start menu is an **overlay** on the town map, not a separate level. The controls list is **read-only** (no rebinding). The mage staff/wand has **no held mesh yet** (graybox/hidden) — it still casts.
