# Test Plan — Item icons, interaction, minimap, skill tree

Covers everything added since the Phase 2.5/3 test plan: 3D item icons, potion meshes + recolor,
E-to-interact (prompt + pickup), the fog-of-war minimap, and the Phase 4 skill tree (4a/4b/4c).

**Setup:** Rebuild the editor target, open the project, **Play**. To test a fresh profile, delete
`Saved/SaveGames/DungeonProfile.sav` first. Kill crabs to gain XP / skill points.

## 1. 3D item icons
- [ ] Inventory (**I**) and hotbar slots show a **rendered 3D thumbnail** of each item (sword,
      crossbow, potions), not a flat color square; background is transparent with the rarity color behind.
- [ ] Looted/started items (e.g. the **starting Crossbow in slot 2**) show the correct icon.
- [ ] Icons are reasonably framed (item fills the slot, not tiny/clipped).

## 2. Potions (shared mesh + texture recolor)
- [ ] In the start room, **three potions sit on the table**: Health (red), Mana (blue),
      Stamina (green) — same bottle mesh, different color.
- [ ] Their inventory icons match those colors once picked up.
- [ ] (Editor) `MI_Potion`'s parent material exposes the **`BaseColorTex`** parameter; renaming was
      `SK_Potion → SM_Potion` (static mesh).

## 3. Interaction prompt + E to pick up
- [ ] Looking at a chest shows a centered **`[E] Open`** prompt below the crosshair; looking away hides it.
- [ ] Looking at a ground item shows **`[E] Pick up`**; pressing **E** collects it (it no longer
      auto-collects on walk-over).
- [ ] While a loot pane is open the prompt reads **`[E] Close`**, and E closes it.
- [ ] Dropping an item from the inventory spawns it in the world; it shows the item's real mesh and
      requires **E** to pick back up.

## 4. Minimap (fog of war)
- [ ] A framed minimap sits in the **top-right**; it starts mostly dark.
- [ ] Walking **reveals** walkable cells around you (circular reveal); unexplored stays dark.
- [ ] The **boss room tints red** once revealed; a **cyan dot** tracks your position.
- [ ] No crash on play (the texture-update fix); regenerating the dungeon resets the fog.

## 5. Skill tree — allocation (Phase 4a/4b)
- [ ] Press **K**: a centered panel shows three columns (**Melee / Ranged / Mage**) with a
      **Points: N** header that matches your unspent skill points.
- [ ] Tier-0 nodes are clickable; clicking one **spends a point**, the node turns green, and the
      counter drops. Deeper nodes are **locked** until their prerequisite is allocated.
- [ ] Multi-rank nodes show **rank x/max** and can be allocated repeatedly.
- [ ] With 0 points, no node is allocatable.
- [ ] Allocations + their effects **persist** across quit & relaunch.

## 6. Skill tree — passive bonuses (4a)
- [ ] Allocating a **+max health / mana / stamina** node visibly **raises the matching HUD bar's max**.
- [ ] Allocating a **damage** node increases that style's damage (crabs die faster).

## 7. Skill tree — combat modifiers (4c)
- [ ] **Melee → Frenzy:** swings come out faster (shorter cooldown).
- [ ] **Melee → Bloodthirst:** landing melee hits **heals** you a little.
- [ ] **Ranged → Split Shot / Mage → Arcane Barrage:** attacks fire **multiple bolts in a spread**.
- [ ] **Ranged → Rapid Reload / Mage → Efficiency:** shots cost **less stamina / mana**.

## 8. Regression
- [ ] HUD bars, level, inventory, hotbar, collection log still work and persist.
- [ ] Combat styles still switch with the equipped weapon; chests still loot; dungeon still generates
      with the boss in its room.
- [ ] No bright light band along the **tops of walls** (Lumen seam fix).
