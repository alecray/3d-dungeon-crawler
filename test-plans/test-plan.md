# Consolidated Test Plan

Untested / unchecked work is up top (**TO TEST**); sections you've already passed are at the bottom
(**PASSED**). **(NEW)** = added since the last pass and never run.

**Setup**
- Rebuild the editor target, open the project, **Play**.
- The game boots to **L_Town**; the start menu appears first.
- To test a fresh profile / first-time cinematics, delete `Saved/SaveGames/DungeonProfile.sav` first.
- Kill crabs for XP / skill points. Enter the dungeon via the town portal.
- Tip: use the **Dev Menu** (Esc → Dev Menu: God Mode, Reveal Map, Teleport Home) to test faster.

---

# TO TEST

## 1. Boss encounter sequence
First-ever kill of the boss (fresh save):
- [ ] Walking into the boss room triggers the encounter: **input locks**, the boss **spawns on the far
      side opposite you**, facing you.
- [ ] The **camera blends to frame the boss** with a **screen shake** while it plays its rise/roar
      **spawn animation**, then blends back to you and control returns.
- [ ] The **entrance doorways seal** with barriers that rise from the floor (you can't leave).
Replays (same boss again):
- [ ] Doors still seal and the boss still plays its spawn animation, but **no camera blend / no shake**.
On death:
- [ ] The sealed doors **sink back down**; the **return portal activates** near room center.

## 2. Boss room — double-height + camera (NEW)
- [ ] The boss room is **two blocks tall** (taller walls + raised ceiling).
- [ ] Doorways into the room have a **lintel above the opening** — no gap/void visible above the door.
- [ ] The intro camera is **straight-on and fairly level** and **doesn't clip the ceiling**.

## 3. Boss fight mechanics
- [ ] **Movement:** the boss **scuttles sideways / circles** you, then **telegraphs (pauses) and lunges**. Smooth, no jitter.
- [ ] **Navmesh hand-off:** break line of sight behind a wall → boss **paths around** it, then resumes scuttling.
- [ ] **Phases:** at ~66% and ~33% HP it **morphs** + buffs; an on-screen "PHASE N" message shows.
- [ ] **Back weak point (phase 1 only):** marker on its back; **hits from behind deal ~2×** (bigger orange numbers). Marker gone after p1.
- [ ] **Projectiles:** spits a bolt (3-bolt spread p2+); **slow enough to dodge**, **damages you** on hit.
- [ ] **Ground slam**, **summon adds (real crab types, p2+)**, **bubble pools (p2-3)**, **enrage (p3)**.
- [ ] **Shell-retreat (p3):** tucks in — immobile + **invulnerable**, hits **clang off** for a few seconds, then re-emerges.

## 4. Floating damage numbers (re-test — was reported missing, now bigger/brighter)
- [ ] Hitting any enemy spawns a **floating number** of the damage; **big, bright gold**, drifts up and fades.
- [ ] Numbers **face the camera** as you move.
- [ ] **Weak-point / boosted hits** show **larger and orange**.

## 5. Dev menu (NEW)
- [ ] **Esc → Dev Menu** opens a panel: No Clip / God Mode / Reveal Map / Kill Player / Teleport Home.
- [ ] **No Clip** — fly through walls (label toggles ON/OFF); turning it off restores normal collision.
- [ ] **God Mode** — invulnerable (label ON/OFF); enemies/traps/bubbles can't hurt you while on.
- [ ] **Reveal Map** — fills in the entire minimap.
- [ ] **Kill Player** — kills you even with God Mode on → death flow runs.
- [ ] **Teleport Home** — fades to black and returns to town.

## 6. Game feel / juice
- [ ] Landing a **melee** hit produces a brief **hit-stop** — swings feel weighty.
- [ ] **Camera kicks** on a melee hit; **lighter recoil** when firing ranged/mage.
- [ ] Enemies get **knocked back** (with a small hop) away from your hits.
- [ ] Below ~35% HP the **screen edges pulse red**, stronger as HP drops; clears when healed.
- [ ] None of it is nauseating / overlong.

## 7. Traps
- [ ] **Spike floors** in corridors rise/fall on a timer and **damage you when up**.
- [ ] **Pressure plates** — stepping on one **telegraphs then erupts spikes once**, then re-arms.
- [ ] **Dart shooters** fire a **dart streak across the passage**; **damages you** if it hits.
- [ ] Traps appear in corridors (and occasionally rooms) but **not in the spawn room**.

## 8. Room-type variety
- [x] Rooms flagged by a **colored marker light**: **gold = Treasure**, **red = Ambush**, **green = Rest**, **purple = Elite**.
- [ ] **Treasure** = 2-3 chests + guards; **Rest** = no monsters/traps; **Ambush** = bigger swarm; **Elite** = one mini-boss + chest.
- [ ] Elites are **tough but fair** (~4× a normal crab's HP).

## 9. Generation tuning (NEW)
- [ ] Rooms are **noticeably bigger** than before.
- [ ] Hallways are **shorter** (rooms connect to their nearest neighbor, not a long chain).
- [ ] **More items / clutter** are distributed around rooms and corridors; more chests overall.
- [ ] Hallway props **don't clip into the walls**.

## 10. Props
- [ ] Rooms are **busier** with clutter.
- [ ] **Corridors have props** hugging the walls (don't block the path).
- [ ] **Coffins always stand against a wall** (never free-standing mid-floor).

## 11. FPS counter (NEW)
- [ ] An **FPS counter** sits **top-right, below the minimap**, color-coded green / yellow (<60) / red (<30).

## 12. Pause menu layout (NEW)
- [ ] Esc menu reads cleanly — **not squished**, buttons are a proper height, and it's **wide enough**
      that the Controls rows fit; Settings panel grows to fit (no clipping).

## 13. Melee stamina
- [ ] A melee swing **drains stamina**; when **out of stamina** it **doesn't fire**.
- [ ] Skill nodes that reduce stamina cost also reduce the melee cost.

## 14. Item icons
- [ ] Inventory (**I**) / hotbar slots show a **3D thumbnail** per item on a transparent bg with rarity color behind.
- [ ] Starting **Crossbow** shows its correct icon; icons well-framed.

## 15. Potions (shared mesh + recolor)
- [ ] Three potions on the **start-room table**: Health (red), Mana (blue), Stamina (green); icons match.

## 16. Interaction prompt + E to pick up
- [ ] Chest shows **`[E] Open`**; ground item shows **`[E] Pick up`** (no auto-collect); loot pane shows **`[E] Close`**.
- [ ] Dropping an item spawns it with its real mesh; **E** picks it back up.

## 17. Skill tree (K)
- [ ] **K** opens Melee/Ranged/Mage columns + **Points: N**; allocate/lock/multi-rank logic works.
- [ ] Passive bonuses (max HP/mana/stamina, damage) + modifiers (Frenzy, Bloodthirst, Split Shot, Rapid Reload) work.
- [ ] **Capstones (Q):** Whirlwind / Volley / Arcane Nova; Q does nothing if locked/cooldown/unaffordable.
- [ ] Allocations + effects **persist** across relaunch.

## 18. Paperdoll equipment
- [ ] Inventory shows an **Equipment** panel; drag to equip (mismatches rejected), click to unequip; bonuses apply + persist.
- [ ] Chest loot view shows the grid only.

## 19. Enemy pathing
- [ ] Crabs **path around walls/corners**, then close in and attack; no stutter. (Needs a full editor restart.)

## 20. Town, shop & portals
- [ ] Starts in **L_Town**; Portal (`[E] Enter`) loads a dungeon (with a fade); ShopNPC buys/sells.
- [ ] Start-room return portal is flush on a wall; gold/inventory/equipment/skills survive transitions.

## 21. Art / feel pass
- [ ] Chests = SK_Chest (open/close anims); torches face the room w/ warm light; portals = spinning cyan gate.
- [ ] Crab death pops/tumbles/shrinks + poof; no bright Lumen band along wall tops.

## 22. Regression
- [ ] HUD bars + level correct; sprint drains stamina; styles switch with weapon; chests loot; dungeon generates.
- [ ] Inventory / hotbar / collection log / level / XP / gold persist across relaunch.

---

# PASSED (already verified)

- [x] **Start screen** — menu on launch, Start fades in, Quit exits, only once per session.
- [x] **Scene transitions** — black fades on town↔dungeon + death restart, fade-in on arrival.
- [x] **Controls in settings** — full scrollable controls list under the sliders.
- [x] **Boss health bar** — top-center red bar, labelled, drains, removed on death.
- [x] **Minimap (fog of war)** — top-right, reveals as you walk, boss room red, cyan player dot, no crash.

---

## Known / expected rough edges (not bugs)
- Everything is **graybox** — the boss isn't a crab yet; doors/portal/traps/bubbles/room-markers are primitive.
- The start menu is an **overlay on the town map**, not a separate menu level.
- The controls list is **read-only** (no rebinding yet).
- Mage style still needs a weapon item to fully exercise.
