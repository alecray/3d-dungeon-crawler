# Test Plan — Phase 2.5 (Equipment & Action Bar) + Phase 3 (Combat & Crab)

Covers the untested work since the last Phase-1 play-test.

**Setup:** Reopen editor → Play `L_DungeonTest`. To test a fresh profile, delete
`Saved/SaveGames/DungeonProfile.sav` first.

## 1. Action bar & equipping
- [x] 8-slot bar shows bottom-center; slot 1 has a **Sword**, highlighted gold; sword visible in hand.
- [x] Open inventory (**I**), drag a weapon (loot a **Crossbow**/**Iron Sword**, or drag the starter) onto
      another hotbar slot → it appears there.
- [x] Press **1–8** (and click slots) → active highlight moves and the **held weapon swaps** (Sword ↔ Crossbow).
- [x] Equipped weapon persists after **quit + relaunch**.

## 2. Combat styles
- [x] **Sword (melee):** LMB plays the swing, sweeps in front, damages crabs (scales with Strength).
- [x] **Crossbow (ranged):** LMB fires a bolt, plays `A_Crossbow_Shoot`, **drains stamina**, bolt flies
      straight and damages the first crab it hits; destroys on walls.
- [x] Out of stamina → ranged attack doesn't fire.

## 3. Crab enemy (replaces graybox humanoid)
- [x] All enemies are **crabs at ~half size**; **hitbox matches** the visible crab (hits connect where expected).
- [x] **Idle** when standing, **Run** (looped) while chasing, **Attack** anim when it hits you.
- [x] Crab orientation/height look right while moving (flag if it faces wrong or floats/sinks).
- [x] On death: **particle poof** bursts and the crab settles into **idle** (placeholder), then despawns
      after a few seconds; you gain XP.

## 4. Loot pane (drag-to-loot)
- [x] Walk to a chest, **E** → lid flips; **chest grid (left)** + **inventory (right)** appear.
- [x] **Drag items chest→inventory** to loot (and inventory→chest to deposit); cross-container drag works.
- [x] **E** again (or walking away) closes both panels; cursor re-locks.

## 5. Drop to world
- [x] Drag an inventory item onto empty space → it **drops** as a spinning pickup at your feet.
- [x] Walk over any pickup → it's **auto-collected** back into the inventory.

## 6. Persistence & regression
- [x] Inventory contents, hotbar assignments, collection log, level/XP/gold survive **relaunch**.
- [x] HUD bars (HP/Mana/Stamina) + level correct; sprint still drains stamina.
- [x] Dungeon still generates (rooms/halls, props, torches, floor/wall/ceiling); the **boss** still works
      in its room (unchanged; graybox + morph cubes).

## Known / expected rough edges (not bugs)
- UI panels are dark boxes with colored cells (no item icons/text yet).
- Mage style has no weapon item yet.
- Bolt spawns from the camera, not a crossbow muzzle socket.
- The boss isn't a crab.
