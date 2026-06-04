# Consolidated Test Plan — all untested work

One plan covering everything not yet play-tested: the latest sessions' work (start screen, scene
transitions, controls reference, full boss encounter + fight mechanics, damage numbers, game-feel juice,
traps, room-type variety, prop changes, melee stamina, boss health bar) **plus** the still-untested
earlier features (item icons, potions, interaction, minimap, skill tree, paperdoll, pathing, town/shop,
art pass).

**Setup**
- Rebuild the editor target, open the project, **Play**.
- The game boots to **L_Town**; the start menu appears first (see §1).
- To test a fresh profile / first-time cinematics, delete `Saved/SaveGames/DungeonProfile.sav` first.
- Kill crabs for XP / skill points. Enter the dungeon via the town portal.

> Items marked **(NEW)** are from the latest sessions and have never been run. The rest were built
> earlier but were never checked off in the prior plans.

---

## 1. Start screen (NEW)
- [ ] On launch a graybox menu fills the screen: title **DUNGEON CRAWLER** + **Start Game** / **Quit**.
- [ ] Nothing of the world shows behind it; the player can't move while it's up.
- [ ] **Start** → menu closes and the world **fades in from black**; you have control in town.
- [ ] **Quit** → the game exits.
- [ ] Returning to town from the dungeon does **not** re-show the menu (only once per launch).

## 2. Scene transitions — black fades (NEW)
- [ ] Entering the dungeon via the town portal **fades to black, then fades in** on the dungeon.
- [ ] Returning to town via a portal does the same.
- [ ] **Dying** fades to black, then fades in on the fresh run.

## 3. Controls reference in settings (NEW)
- [ ] Pause (**Esc**) → **Settings** shows a scrollable **Controls** list under the sliders.
- [ ] Every binding is present: WASD move, mouse look, Space jump, Shift sprint, LMB attack, Q ability,
      1-8 hotbar, E interact, I/C/K menus, Esc pause.
- [ ] The list fits / scrolls cleanly inside the panel.

## 4. Boss encounter sequence (NEW)
First-ever kill of the boss (fresh save):
- [ ] Walking into the boss room triggers the encounter: **input locks**, the boss **spawns on the far
      side opposite you**, facing you.
- [ ] The **camera blends to frame the boss** (~1.8s) with a **screen shake** while it plays its
      rise/roar **spawn animation**, then blends back to you and control returns.
- [ ] The **entrance doorways seal** with barriers that rise from the floor (you can't leave).
Replays (same boss again):
- [ ] Doors still seal and the boss still plays its spawn animation, but there's **no camera blend and
      no shake** — straight into the fight.
On death:
- [ ] The sealed doors **sink back down**; the **return portal activates** (cyan gate) near room center.

## 5. Boss health bar (NEW)
- [ ] A **red boss health bar** appears pinned **top-center** when the fight starts, labelled
      (e.g. **HERMIT CRAB**).
- [ ] It **drains as you damage the boss**, and is **removed when the boss dies**.

## 6. Boss fight mechanics (NEW)
- [ ] **Movement:** the boss **scuttles sideways / circles** you, then **telegraphs (pauses) and lunges**
      forward. Smooth, no jitter.
- [ ] **Navmesh hand-off:** break line of sight behind a wall/pillar → the boss **paths around** it,
      then resumes scuttling when it sees you (no getting stuck).
- [ ] **Phases:** at ~66% and ~33% HP it **morphs** + buffs; an on-screen "PHASE N" message shows.
- [ ] **Back weak point (phase 1 only):** a bright marker is on its back; **hits from behind deal ~2×**
      (bigger orange damage numbers). The marker disappears after phase 1.
- [ ] **Projectiles:** it spits a bolt at you (3-bolt spread in phase 2+); bolts are **slow enough to
      dodge** and **damage you** on hit.
- [ ] **Ground slam:** a close-range hit when you're near.
- [ ] **Summon adds (phase 2+):** spawns minions that are **real crab-type monsters** (not blank boxes).
- [ ] **Bubble pools (phase 2-3):** a cluster of damaging floor pools erupts around you; popping after
      a few seconds.
- [ ] **Enrage (phase 3):** a temporary speed burst.
- [ ] **Shell-retreat (phase 3):** it **tucks in** — immobile and **invulnerable**; your hits **clang off
      with no damage** for a few seconds, then it re-emerges.

## 7. Floating damage numbers (NEW)
- [ ] Hitting any enemy spawns a **floating number** of the damage dealt; it **drifts up and fades/shrinks**.
- [ ] Numbers **face the camera** as you move.
- [ ] **Weak-point / boosted hits** show **larger and orange**.

## 8. Game feel / juice (NEW)
- [ ] Landing a **melee** hit produces a brief **hit-stop** (a few frames of freeze) — swings feel weighty.
- [ ] The **camera kicks** on a melee hit, and a **lighter recoil** when firing ranged/mage.
- [ ] Enemies get **knocked back** (with a small hop) away from your hits.
- [ ] Below ~35% HP the **screen edges pulse red**, faster/stronger as HP drops; it clears when you heal.
- [ ] None of it is nauseating / overlong (flag if hit-stop is too long or the kick too strong).

## 9. Traps (NEW)
- [ ] **Spike floors** in corridors rise/fall on a timer and **damage you when up**.
- [ ] **Pressure plates** — stepping on one **telegraphs then erupts spikes once**, then re-arms.
- [ ] **Dart shooters** on corridor walls fire a **dart streak across the passage**; it **damages you** if it hits.
- [ ] Traps appear in corridors (and occasionally rooms) but **not in the spawn room**.

## 10. Room-type variety (NEW)
- [ ] Rooms feel distinct, flagged by a **colored marker light**: **gold = Treasure**, **red = Ambush**,
      **green = Rest (safe)**, **purple = Elite** (Normal rooms have no marker).
- [ ] **Treasure** rooms have **2-3 chests** and are **guarded**; **Rest** rooms have **no monsters/traps**;
      **Ambush** rooms have a **bigger swarm**; **Elite** rooms have **one tough mini-boss + a chest**.
- [ ] Elites are **tough but fair** (~4× a normal crab's HP); the marker colors read at a glance.

## 11. Props (NEW changes)
- [ ] Rooms are **noticeably busier** with clutter than before.
- [ ] **Corridors now have props**, hugging the walls (they don't block the path).
- [ ] **Coffins always stand flush against a wall** (never free-standing mid-floor).

## 12. Melee stamina (NEW)
- [ ] A melee swing now **drains stamina** (watch the stamina bar).
- [ ] When **out of stamina**, the swing **doesn't fire** (matches ranged/mage).
- [ ] Skill nodes that reduce stamina cost also reduce the melee cost.

## 13. Item icons
- [ ] Inventory (**I**) and hotbar slots show a **rendered 3D thumbnail** per item on a transparent
      background with the rarity color behind — not a flat square.
- [ ] The starting **Crossbow in slot 2** shows its correct icon; icons are well-framed.

## 14. Potions (shared mesh + recolor)
- [ ] Three potions sit on the **start-room table**: Health (red), Mana (blue), Stamina (green).
- [ ] Their inventory icons match those colors once picked up.

## 15. Interaction prompt + E to pick up
- [ ] Looking at a chest shows a centered **`[E] Open`**; looking away hides it.
- [ ] Looking at a ground item shows **`[E] Pick up`**; **E** collects it (no auto-collect on walk-over).
- [ ] While a loot pane is open the prompt reads **`[E] Close`** and E closes it.
- [ ] Dropping an item spawns it in the world with its real mesh; **E** picks it back up.

## 16. Minimap (fog of war)
- [ ] A framed minimap sits **top-right**, starting mostly dark.
- [ ] Walking **reveals** walkable cells around you; unexplored stays dark.
- [ ] The **boss room tints red** once revealed; a **cyan dot** tracks you. No crash; regenerating resets fog.

## 17. Skill tree (K)
- [ ] **K** opens a centered panel: **Melee / Ranged / Mage** columns + **Points: N** matching unspent points.
- [ ] Tier-0 nodes clickable; clicking **spends a point**, turns green, decrements; deeper nodes **locked**
      until prereq allocated; multi-rank nodes show **rank x/max**; 0 points = nothing allocatable.
- [ ] **+max health/mana/stamina** nodes raise the matching HUD bar max; **damage** nodes make crabs die faster.
- [ ] **Frenzy** (faster swings), **Bloodthirst** (melee heals), **Split Shot/Arcane Barrage** (multi-bolt),
      **Rapid Reload/Efficiency** (cheaper shots) all work.
- [ ] **Capstones (Q):** Whirlwind (melee AoE, stamina, ~6s), Volley (bolt burst, stamina, ~5s),
      Arcane Nova (mana AoE, ~7s). Q does nothing if locked/cooldown/unaffordable.
- [ ] Allocations + effects **persist** across quit & relaunch.

## 18. Paperdoll equipment
- [ ] Inventory (**I**) shows an **Equipment** panel (head/amulet/body/gloves/belt/legs/feet + 4 rings).
- [ ] **Drag** armor/accessory onto its matching slot to equip; mismatched items won't equip; equipping
      raises the matching HUD bar/damage; **click** a slot to unequip.
- [ ] Equipment + bonuses **persist** across relaunch and the town↔dungeon trip.
- [ ] The chest loot view shows the grid only (no equipment panel).

## 19. Enemy pathing
- [ ] Crabs **path around walls/corners** to reach you, then close in and attack; smooth, no stutter.
      (Requires a full editor restart for the navmesh config.)

## 20. Town, shop & portals
- [ ] Game starts in **L_Town**; the **Portal** (`[E] Enter`) loads a fresh dungeon (now with a fade).
- [ ] The **ShopNPC** (`[E] Shop`) opens buy/sell; gold + inventory adjust.
- [ ] Dungeon **start-room return portal** is flush on a wall (not blocking a door) and returns to town.
- [ ] Gold / inventory / equipment / skills survive town↔dungeon transitions.

## 21. Art / feel pass
- [ ] Chests are **SK_Chest** (closed at rest); open/close anims play with the loot menu.
- [ ] Wall **torches** (SM_Torch) face into the room, sit low, warm localized light.
- [ ] Portals look like a spinning cyan **energy gate**.
- [ ] Crab **death**: pops up, tumbles, shrinks away (no lingering corpse) + particle poof.
- [ ] No bright light band along the **tops of walls** (Lumen seam).

## 22. Regression
- [ ] HUD bars (HP/Mana/Stamina) + level correct; **sprint still drains stamina**.
- [ ] Combat styles switch with the equipped weapon; chests still loot; dungeon still generates.
- [ ] Inventory / hotbar / collection log / level / XP / gold persist across relaunch.

---

## Known / expected rough edges (not bugs)
- Everything is **graybox** — the boss isn't a crab yet; doors/portal/traps/bubbles/room-markers are
  primitive shapes/lights.
- The start menu is an **overlay on the town map**, not a separate menu level.
- The controls list is **read-only** (no rebinding yet).
- Boss bolts spawn from a point in front of the boss, not an authored muzzle.
- Mage style still needs a weapon item to fully exercise.
