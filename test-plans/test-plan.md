# Consolidated Test Plan

Untested / unchecked work is up top (**TO TEST**); sections you've already passed are at the bottom
(**PASSED**). **(NEW)** = added since the last revision and never run.

**Setup**
- Rebuild the editor target, open the project, **Play**.
- The game boots to **L_Town**; the start menu appears first.
- To test a fresh profile / first-time cinematics, delete `Saved/SaveGames/DungeonProfile.sav` first.
- Kill crabs for XP / skill points. Enter the dungeon via the town portal.
- Tip: **Dev Menu** (Esc → Dev Menu) has **Teleport to Boss**, God Mode, Reveal Map, Teleport Home for fast testing.

---

# TO TEST

## 0. This session's changes (NEW — test these first)
**Dash (Shift):**
- [ ] Shift does a quick **dash** in your movement direction (or facing when standing still), **not** a sprint.
- [ ] It **costs stamina** and has a short cooldown; spamming is gated. With no stamina the **stamina bar flashes red** and you don't dash.

**Insufficient-resource feedback:**
- [ ] Swinging/firing with no stamina (or casting a spell with no mana) **flashes the matching HUD bar red** instead of silently doing nothing.

**Boss combat (anim-test mode — no specials):**
- [ ] The boss **swings its attack animation** when in range, and the hit **lands partway through the swing** (frame 20) — you can **step/dash out of the way to dodge it**.
- [ ] A clean hit is **brutal (~75% of your HP)**. It closes distance **quickly**.
- [ ] On death it **drops several items** at the kill spot (each with a **rarity-colored beam**), and the **return portal appears elsewhere in the room** (a back corner), not on the loot.
- [ ] The boss **spawn-in** plays a code VFX: ground flare + an energy pillar + a rising shard swirl + a debris ring. No phase changes / specials (that's expected — anim-test mode).
- [ ] **Boss room** has **no scenery except a banner on each side of every doorway**, and is **two walls tall** (raised ceiling) with no clipping between the wall courses.

**VFX / feel quick wins:**
- [ ] **Footstep dust** puffs at your feet as you move on the ground.
- [ ] **Enemy death** sinks into the floor + shrinks (dissolve), not a pop-up.
- [ ] **Picking up** a dropped item throws a rarity-colored **sparkle**; opening a **chest** pops a rarity-colored burst (brighter for Rare+).
- [ ] **Traps glow red** (pulsing) just before they fire — spikes ramp before rising, plates flash on the step, dart shooters glow before a shot.
- [ ] **Wall torches** sit a bit lower and their light comes from the **flame tip**, not the middle of the stick.

## 1. Hermit-crab boss — skeletal mesh + anims (NEW)
- [ ] The boss is the **hermit-crab skeletal mesh** (not graybox cubes), boss-sized, upright, facing you.
- [ ] It plays an **idle** animation standing still and a **walk** animation while moving/chasing.
- [ ] It **stands on the floor** — not buried/sunk into the ground.
- [ ] Expected gaps (not bugs): no attack/spawn anims yet, so no swing animation on hit and the intro
      "rise" doesn't show on the crab (the camera/shake still play).

## 2. Boss room + intro camera (UPDATED — now a normal room)
- [ ] The boss room is just a **normal (big) room** like the others — single height, normal walls/doorways.
      No double-height, no stacked-wall pillars, no weird bars.
- [ ] **No floor traps** inside the boss arena.
- [ ] The intro camera frames the boss roughly **straight-on** and **doesn't clip the ceiling**.

## 3. Boss encounter sequence
First-ever kill of the boss (fresh save):
- [ ] Walking in triggers it: **input locks**, the boss **spawns opposite you**, camera **blends to frame it**
      with a **screen shake**, then blends back and control returns.
- [ ] The **entrance doorways seal** with rising barriers (you can't leave).
Replays / on death:
- [ ] Replays: doors seal but **no camera blend / no shake**.
- [ ] On death: doors **sink back down**; the **return portal activates**.
- [ ] No "Phase 9" / multiple bosses — regenerating the dungeon no longer leaves old bosses around.

## 4. Boss fight mechanics
- [ ] **Movement:** scuttles sideways / circles you, then **telegraphs + lunges**. Smooth, no jitter.
- [ ] **Navmesh hand-off:** break line of sight behind a wall → it **paths around**, then resumes scuttling.
- [ ] **Phases:** at ~66% and ~33% HP it buffs; a "PHASE 2/3" message shows (only ever 1→2→3).
- [ ] **Back weak point (phase 1):** hits from behind deal **~2×** (bigger orange numbers); marker gone after p1.
- [ ] **Projectiles** (3-bolt spread p2+, dodgeable), **ground slam**, **summon adds**, **bubble pools (p2-3)**,
      **enrage (p3)**, **shell-retreat (p3, invulnerable for a few sec)**.

## 5. Scenery — deliberate placement (NEW)
- [ ] Each room feels **themed** (storage / dining / crypt / smithy / library) — props from one coherent set.
- [ ] Furniture **lines the walls** flush + facing into the room (no random spin); the centre stays walkable.
- [ ] **Corner stacks** — tight piles in corners; some **dining halls** have a **table ringed by stools**.
- [ ] **Coffins lie flat along the wall** (rotated 90°, not jutting into the room).
- [ ] **No scenery on traps**, and **no scenery spawning inside other scenery**.
- [ ] Corridors have wall-hugging props that don't block the path.
- [ ] (Known: the **weapon rack** still spawns tilted/sunk — asset pivot fix pending, on the art TODO.)

## 6. VFX / "alive" pass (NEW)
- [ ] **Impact sparks** — hitting an enemy throws a small burst of bits + a brief light flash (weak-point = orange).
- [ ] **Ambient dust** — faint motes drift in the air and catch the torch light.
- [ ] **Damage flash** — the screen pulses red the instant you take a hit.
- [ ] **Level-up burst** — a gold poof erupts off you when you level up.
- [ ] **Low-HP screen** — chromatic aberration + desaturation ramp up as health drops (on top of the red vignette).
- [ ] **Sprint FOV** — the view widens a touch while sprinting.
- [ ] **FPS holds** with all of it on (the dust is pooled; impacts are cheap).

## 7. Game feel / juice
- [ ] Landing a **melee** hit produces a brief **hit-stop**; **camera kick** on hit, light recoil on ranged fire.
- [ ] Enemies get **knocked back** (small hop) away from your hits.
- [ ] None of it is nauseating / overlong (flag if hit-stop too long, kick too strong, dust too thick).

## 8. Dev menu (NEW)
- [ ] **Esc → Dev Menu**: No Clip / God Mode / Reveal Map / **Teleport to Boss** / Kill Player / Teleport Home.
- [ ] **No Clip** fly through walls (ON/OFF label); **God Mode** invulnerable; **Reveal Map** fills the minimap.
- [ ] **Teleport to Boss** drops you in the boss room; **Kill Player** kills you even in God Mode; **Teleport Home** → town.

## 9. Shop & inventory (NEW)
- [ ] Your **inventory shows "Gold: N"** (your own inventory screen, not the chest loot grid).
- [ ] Opening the **shop closes the inventory and vice-versa** — they never overlap (shop has its own buy/sell/gold).

## 10. Traps
- [ ] **Spike floors** rise/fall and damage you when up; **pressure plates** telegraph then erupt once;
      **dart shooters** fire across the passage. Not in the spawn room or the boss room.

## 11. Room-type variety
- [x] Rooms flagged by a **colored marker light**: gold = Treasure, red = Ambush, green = Rest, purple = Elite.
- [ ] Treasure = 2-3 chests + guards; Rest = no monsters/traps; Ambush = bigger swarm; Elite = one tough mini-boss + chest.

## 12. Generation tuning
- [ ] Rooms are **bigger**; hallways are **shorter** (nearest-neighbor); more chests; props don't clip walls.

## 13. Skill tree (K) — declutter (NEW)
- [ ] **K** opens Melee/Ranged/Mage columns + **Points: N**; each node is **one clean line** (name + rank),
      and **hovering shows the full description in a tooltip** (no more overlapping text).
- [ ] Allocate/lock/multi-rank works; passives + modifiers + **Q capstones** work; persists across relaunch.

## 14. Floating damage numbers
- [ ] Hitting an enemy spawns a **big bright gold** number that faces the camera and drifts up; weak-point = larger/orange.

## 15. Melee stamina
- [ ] A melee swing **drains stamina**; **out of stamina** = no swing. Skill cost-reduction also lowers melee cost.

## 16. FPS counter
- [ ] An **FPS counter** sits **top-right, below the minimap**, color-coded green / yellow (<60) / red (<30).

## 17. Pause menu layout
- [ ] Esc menu reads cleanly — not squished, proper button height, wide enough for the Controls rows.

## 18. Item icons / Potions
- [ ] Inventory + hotbar show **3D thumbnails** per item; the three start-room **potions** (red/blue/green) match their icons.

## 19. Interaction prompt + E
- [ ] `[E] Open` on chests, `[E] Pick up` on ground items (no auto-collect), `[E] Close` on loot panes; dropped items show their mesh.

## 20. Paperdoll equipment
- [ ] Drag to equip (mismatches rejected), click to unequip; bonuses apply + persist; chest view = grid only.

## 21. Enemy pathing
- [ ] Crabs **path around walls/corners**, then close in and attack; no stutter. (Needs a full editor restart.)

## 22. Town, shop & portals
- [ ] Starts in **L_Town**; Portal (`[E] Enter`) loads a dungeon with a fade; ShopNPC buys/sells; return portal flush on a wall;
      gold/inventory/equipment/skills survive transitions.

## 23. Regression
- [ ] HUD bars + level correct; sprint drains stamina; styles switch with weapon; chests loot; dungeon generates.
- [ ] Inventory / hotbar / collection log / level / XP / gold persist across relaunch. No bright Lumen band on wall tops.

---

# PASSED (already verified)

- [x] **Start screen** — menu on launch, Start fades in, Quit exits, only once per session.
- [x] **Scene transitions** — black fades on town↔dungeon + death restart, fade-in on arrival.
- [x] **Controls in settings** — full scrollable controls list under the sliders.
- [x] **Boss health bar** — top-center red bar, labelled, drains, removed on death.
- [x] **Minimap (fog of war)** — top-right, reveals as you walk, boss room red, cyan player dot, no crash.

---

## Known / expected rough edges (not bugs)
- Still graybox in places — doors/portal/traps/bubbles/room-markers are primitive shapes; the crab boss has
  only idle/walk anims so far (no attack/spawn/death anims yet).
- The **weapon-rack** prop spawns tilted/sunk (asset pivot fix is on the art TODO).
- The start menu is an **overlay on the town map**, not a separate menu level.
- The controls list is **read-only** (no rebinding yet); Mage style still needs a weapon item to fully exercise.
