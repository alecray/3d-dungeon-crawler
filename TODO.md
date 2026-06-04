# TODO

> Open / untested work is up top. Completed items are in **Done** at the bottom.

---

# OPEN

## Gameplay

- [ ] Adjust the death flow (DESIGN DECISION pending) — currently death keeps all progression and
      reloads the same dungeon, no stakes. Decide: respawn in the dungeon vs. back in town; possibly
      require a consumable "life scroll" to respawn (else lose the run / drop loot). Ties into run-stakes
      + the town-portal difficulty/risk-reward idea in the backlog.
- [ ] Mage weapon — add a staff/wand item so the Mage combat style can be equipped and fully exercised
      (currently mage works only via abilities; no weapon item exists).
- [ ] Trophy cases — placeable displays (e.g. in town) where players can show off items earned from the collection log

## Boss — next

- [ ] VERIFY IN PIE: intro camera framing, hold length, doors line up + actually block, input hands
      back cleanly, boss scuttles/lunges without jitter at the LoS hand-off, double-height room + lintels.
- [ ] Pincer-sweep cone telegraph (replace/augment the radius-only slam).
- [ ] Swap graybox: hermit-crab model, real gate/portcullis door mesh, portal mesh.
- [ ] Support multiple boss types (only one boss id today).
- [ ] (later) Burrow & resurface — boss digs under, re-emerges near the player (not for this boss).

## Flow / UX

- [ ] Rebindable controls — let the player change key bindings from the settings menu (click a row,
      press a new key); persist the remaps in the save profile and apply to the Enhanced Input mappings.
- [ ] (later) Dedicated main-menu level — needs an empty .umap authored in-editor; then point the
      GameMode/GameDefaultMap at it instead of overlaying the boot map.

## RPG depth (genre features not yet implemented)

- [ ] Equipment set bonuses (wearing matching pieces grants extra effects)
- [ ] Character classes / starting archetypes
- [ ] Elemental damage types + resistances/weaknesses
- [ ] Dodge / block / parry defensive options
- [ ] Biomes / themed floors (visual + enemy-set variety per depth)
- [ ] Keys & locked doors, secret rooms, destructible walls
- [ ] Crafting / enchanting / item upgrades (sinks for materials + gold)
- [ ] More consumables beyond potions (scrolls, food, buff items)

## Backlog (bigger systems, not scheduled yet)

- [ ] Enemy behavior archetypes — distinct AI beyond chase+melee: ranged shooter, charger, exploder,
      shielded/flanker (so encounters are about positioning + threat, not just trading hits).
- [ ] Status effects (poison, burn, freeze, stun, bleed) on weapons/abilities/enemies
- [ ] Loot rarity tiers + procedural affixes (rolled item stats: +damage, +crit, resistances, etc.)
- [ ] Multiple dungeon floors with descending, scaling difficulty (floor 1 → N)
- [ ] FTUE for controls — first-run onboarding surfacing movement/attack/interact/abilities/inventory.
- [ ] Fishing mechanic in the town — a relaxing side activity (cast/reel minigame, catch table, maybe sell/cook the catch).
- [ ] Dungeon-select menu on the town portal — lists dungeons/bosses you've cleared; replay at the same
      or harder difficulty tiers for greater reward potential, with the risk of losing everything in the
      run (high-stakes mode). Pairs with run-stakes / meta-progression.

## Engine / Settings

- [ ] Resolve the "Missing Project Settings" editor warning — enable Shader Model 6 (SM6) and/or set
      up Nanite in Project Settings (editor shows a toast about Nanite assets needing SM6)

## Art / VFX

- [ ] Chest model and opening animation
- [ ] Vendor / shop model (currently graybox) — shopkeeper stall / counter
- [ ] Portal model (currently a code-built procedural "energy gate") — proper portal mesh/frame
- [ ] Scenery props — clutter scattered on the floor and on top of tables/surfaces:
  - Tabletop: candles & candlesticks, plates/bowls, mugs/tankards, bottles & flasks,
    books/scrolls, quill + inkpot, food (bread, cheese, apple), coin stacks, gems,
    potion vials, maps/parchment, dice, knives, small pouches

  models to make later:
  - sacks
  - broken pottery shards
  - hay piles
  - candelabra/floor candlesticks
  - lantern
  - chains
  - cobwebs
  - rugs/carpets
  - statues
  - gravestones
  - cage (idk what happened here it wouldnt import, blend has been started)
  - ladder
  - coin/treasure piles

## UI

- [ ] Better item icons — current render-to-texture silhouettes are recognizable but rough
  (flat shading, no fill/depth, weapons-only). Options: tune the icon studio lighting/material for
  nicer 3D thumbnails, add proper icon meshes for all items (potions, gems, etc.), or hand-authored
  2D icon textures per item.

## Audio

- [ ] Ambient music (dungeon background track; later boss-fight / town variants)

## Monster ideas
- eel
- manta ray / sting ray
- ant eater boss
- goat
- bear
- dog
- monkey
- snake

---

# DONE

## Gameplay / combat
- [x] Boss encounter sequence — spawn-on-entry opposite the player; entrance doors seal (open on death);
      first-ever kill plays a camera-focus + screen-shake + intro cinematic (persisted per boss id).
- [x] Boss fight depth — scuttle + telegraphed lunge, hybrid navmesh, projectile volley (3-spread p2+),
      phase-1 back weak point, phase-2/3 bubble pools, shell-retreat (p3), 3 phases + specials.
- [x] Boss health bar — dedicated top-center red bar, labelled from the boss id, removed on death.
- [x] Boss room built double-height (taller walls + raised ceiling + doorway lintels); intro camera
      moved straight-on/lower (no ceiling clip). Projectile speed tuned; summoned adds get a real type.
- [x] Floating damage numbers — world-space 3D text; weak-point hits bigger/orange; big & bright.
- [x] Improve enemy pathing (runtime navmesh + MoveTo).
- [x] Dungeon traps — spike floors, pressure plates, dart shooters (ADungeonTrap).
- [x] Room-type variety — Treasure / Ambush / Rest / Elite / Normal, each with a colored marker light.
- [x] Weapon usage costs stamina (melee now spends stamina, gated when empty).
- [x] Game juice — hit-stop, camera kick, enemy knockback, low-health vignette.
- [x] Dev menu (Esc → Dev Menu) — No Clip, God Mode, Reveal Map, Kill Player, Teleport Home.
- [x] Potions pick up on E press (interact-to-collect).

## Flow / UX
- [x] Start screen — graybox main menu (Start/Quit) over the boot map, once per session.
- [x] Scene transitions — black camera fades on every level travel + fade-in on arrival.
- [x] Controls in settings — full scrollable controls reference in the pause→Settings panel.

## Generation / art
- [x] Bigger rooms + shorter (nearest-neighbor) hallways + denser props/chests + wall props no longer clip.
- [x] Dungeon fog (atmospheric volumetric/height fog).
- [x] In generation: barrel, pots, bucket, rocks, bones, anvil, coffin, mushrooms as clutter;
      weapon racks against walls; banners flanking doorways.
