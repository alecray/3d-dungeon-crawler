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

**Dash i-frames (new gameplay):**
- [ ] Dashing (Shift) grants a brief **invulnerability window** (~first 0.15s of the 0.2s dash): time a dash into a boss swing and you take **no damage** (roll *through* the hit), not just out of its range. After the window you're vulnerable again on the dash's tail.
- [ ] **God Mode** still works and isn't broken by dashing (a dash doesn't switch invulnerability back off when god mode is on).

**Big refactor — regression sweep (behaviour should be UNCHANGED).** The player class was split into
components (fishing, combat) and gold/cheats moved off it; all source was reorganized into subfolders;
enemy assets were relocated. Nothing should *feel* different — verify the systems still work:
- [ ] **Combat** — melee swing + wall-deflect bounce, crossbow bolts (cost stamina), mage bolts (cost mana, released mid-cast), and the **Q abilities** (Whirlwind / Volley / Nova) all still fire; hit-stop + camera kick land on a connecting hit; the hit-marker still flashes.
- [ ] **Weapon swap** still sets the style + held mesh (sword = melee, crossbow = ranged, staff = mage).
- [ ] **Fishing** (town) — cast/idle/tension/reel poses, the status line, the "need a Fishing Rod" gate, and the rod stowing + weapon restoring on finish.
- [ ] **Gold** — shop buy/sell, blackjack bet/payout, and "Give 1,000,000 Gold" all read/update the **same** persisted gold; it survives a relaunch.
- [ ] **Dev cheats** (now on the player controller) — No Clip / God Mode / Kill still work from Esc → Dev Menu.
- [ ] **Crab + boss** still render with their animations (assets moved to `Enemies/Regular` & `Enemies/Bosses`; soft refs resolve).

## 1. Prior session — blackjack / boss / combat overhaul (still untested)

**Blackjack — art + UX overhaul:**
- [ ] It's now a **placed actor in L_Town** (reposition in-editor); sits flat on the floor (no float) and **blocks you** correctly (bounds-fitted box collision); `[E] Play Blackjack` still appears.
- [ ] Cards deal as real **card-face images** on the felt; the dealer's 2nd card is **face-down (back)** until the hand resolves; faces read matte (not blown out).
- [ ] The table is the textured **SM_Blackjack_Table** mesh; the **curved edge faces the player**, flat edge away.
- [ ] Pressing **E** pulls you to a **fixed seat** in front of the table with a consistent camera angle; the prompt hides while seated and returns after Leave.
- [ ] Card layout: **your row nearest you**, the **dealer's row just behind it**, no overlap; the **newest dealt card is on the left** and older cards shift right.
- [ ] Readouts: **"Dealer N" / "You N"** are **3D labels beside each row** (dealer shows the up-card value during your turn, full total on resolve); **"Hit or Stand." / result** is **bottom-center of the screen**; the **left panel** has just Gold / Bet / aligned button rows (Bet± · Deal/Hit/Stand · Leave).

**Boss:**
- [ ] **Attack telegraph is forward** — the red disc sits where the blow lands (in front of the boss), not under it, and the hit matches the disc.
- [ ] **Slower attacks** — a long pause between swings (cooldown 5.6s) so there's a clear punish window.
- [ ] **Boss glows** a little (self-illuminated) so it's readable in the dark.

**Combat feel / bugs:**
- [ ] Damage numbers spawn at the **impact point on the enemy's body** (not above the head), **linger ~2.2s**, rise slowly. Backstab / weak-point = larger + orange.
- [ ] A red **"-N" popup** pops over your **HP bar** when you take damage, then drifts off in a random direction and fades.
- [ ] **Dash distance is consistent** — no longer flings you too far when dashing while moving + holding a key.
- [ ] **Equipping** Max-Health gear **no longer flashes the screen red** (only real damage flashes).

## 2. Carried over (still untested)
- [ ] **Boss attack anim** plays on melee swings, with the hit landing on **frame 20** (dodge by repositioning).
- [ ] **Bonfire** in Rest rooms (green-lit): `[E] Rest` **fully heals + refills mana/stamina** (and saves). Reusable.
- [ ] **Mage weapon** — buy/loot the Apprentice Wand / Staff, select it: LMB casts **spell bolts** that cost **mana** (Mage style).
- [ ] **Backstab** — hitting a regular enemy **from behind** deals extra damage (orange crit number); facing it = normal.
- [ ] **Starting class** (fresh save — delete `DungeonProfile.sav` first) — menu offers **Warrior / Ranger / Mage**; picking seeds stats + a starting weapon; a returning save shows **Continue** instead. Nothing is locked.
- [ ] **Level-up burst** — a gold poof erupts off you when you level up.
- [ ] **Enemy knockback** — enemies get a small hop away from your hits.
- [ ] **Feel check** — none of the juice is nauseating / overlong (hit-stop, camera kick, dust density).

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
- The boss runs in **anim-test mode** (`bAbilitiesEnabled = false`): **no phases/specials** (projectile volley, ground slam, summon adds, bubble pools, enrage, shell-retreat) until re-enabled. No **death/spawn rig** anim yet (spawn uses the code VFX).
- The **weapon-rack** prop spawns tilted/sunk (asset pivot fix is on the art TODO).
- **Navmesh hand-off** is untested — there are no walls in the boss arena yet to break line of sight.
- Still graybox in places (doors / portal / traps / bubbles / room-markers / bonfire). The start menu is an **overlay** on the town map, not a separate level. The controls list is **read-only** (no rebinding). The mage staff/wand has **no held mesh yet** (graybox/hidden) — it still casts.
