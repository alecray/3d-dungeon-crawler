# TODO

## Gameplay


- [ ] Change boss to spawn when you walk into the boss room (instead of at dungeon generation)
- [x] Boss fight depth — crab-like scuttle + telegraphed lunge movement, player-targeting projectile
      volley (3-spread in phase 2+), phase-1 back weak point (2x dmg from behind, marked + auto-hidden
      after p1), and phase-2/3 bubble-pool hazards (ABubbleHazard). Reuses existing phase/special system.
- [x] Floating damage numbers — world-space 3D text on every hit to an enemy; weak-point hits show
      bigger/orange (ADamageNumber, billboards to camera, arcs up + shrinks out).
- [x] Improve enemy pathing (runtime navmesh + MoveTo so enemies path around walls instead of steering straight)
- [x] Dungeon traps — graybox spike floors (periodic), pressure plates (step-triggered spikes), and
      wall-mounted dart shooters. Scattered in corridors + occasional room cells via ADungeonTrap;
      mesh swap-in points (BaseMeshOverride/SpikeMeshOverride/DartMeshOverride + /Game/Traps paths).
- [ ] Weapon usage should cost stamina (melee swing + ranged already partial; unify stamina cost across all weapons)
- [ ] Trophy cases — placeable displays (e.g. in town) where players can show off items earned from the collection log
- [x] Make potions pick up on E press, not automatically (interact-to-collect instead of overlap auto-pickup)

## RPG depth (genre features not yet implemented)

- [ ] Loot rarity tiers + procedural affixes (rolled item stats: +damage, +crit, resistances, etc.)
- [ ] Equipment set bonuses (wearing matching pieces grants extra effects)
- [ ] Character classes / starting archetypes
- [ ] Status effects (poison, burn, freeze, stun, bleed) on weapons/abilities/enemies
- [ ] Elemental damage types + resistances/weaknesses
- [ ] Dodge / block / parry defensive options
- [ ] Multiple dungeon floors with descending, scaling difficulty (floor 1 → N)
- [ ] Biomes / themed floors (visual + enemy-set variety per depth)
- [ ] Keys & locked doors, secret rooms, destructible walls
- [ ] Crafting / enchanting / item upgrades (sinks for materials + gold)
- [ ] More consumables beyond potions (scrolls, food, buff items)

## Engine / Settings

- [ ] Resolve the "Missing Project Settings" editor warning — enable Shader Model 6 (SM6) and/or set
      up Nanite in Project Settings (editor shows a toast about Nanite assets needing SM6)

## Art / VFX

- [ ] Chest model and opening animation
- [ ] Vendor / shop model (currently graybox) — shopkeeper stall / counter
- [ ] Portal model (currently a code-built procedural "energy gate") — proper portal mesh/frame
- [x] Dungeon fog (atmospheric volumetric/height fog for mood + depth)
- [ ] Scenery props — clutter scattered on the floor and on top of tables/surfaces:
  - Floor:
  - Tabletop: candles & candlesticks, plates/bowls, mugs/tankards, bottles & flasks,
    books/scrolls, quill + inkpot, food (bread, cheese, apple), coin stacks, gems,
    potion vials, maps/parchment, dice, knives, small pouches

- [x] in generation now: barrel, pots, bucket, rocks (random scale), bones, anvil, coffin, mushrooms
  (random scale) as floor clutter; weapon racks against walls; banners flanking doorways.

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