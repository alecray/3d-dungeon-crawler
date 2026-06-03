# TODO

## Gameplay

- [ ] Phase 4d — skill-tree active abilities: nodes that unlock usable skills (dash, AoE nova, multishot
      volley, etc.) on a cooldown system bound to a key; workshop the ability list + graybox VFX

- [ ] Change boss to spawn when you walk into the boss room (instead of at dungeon generation)
- [x] Improve enemy pathing (runtime navmesh + MoveTo so enemies path around walls instead of steering straight)
- [ ] Dungeon traps (e.g. spike floors, dart shooters, pressure plates)
- [ ] Weapon usage should cost stamina (melee swing + ranged already partial; unify stamina cost across all weapons)
- [x] Make potions pick up on E press, not automatically (interact-to-collect instead of overlap auto-pickup)

## Engine / Settings

- [ ] Resolve the "Missing Project Settings" editor warning — enable Shader Model 6 (SM6) and/or set
      up Nanite in Project Settings (editor shows a toast about Nanite assets needing SM6)

## Art / VFX

- [ ] Chest model and opening animation
- [ ] Dungeon fog (atmospheric volumetric/height fog for mood + depth)
- [ ] Scenery props — clutter scattered on the floor and on top of tables/surfaces:
  - Floor: barrels, sacks, baskets, clay pots/urns, crates (have), wooden buckets,
    rubble/rock piles, bone piles & skulls, broken pottery shards, hay piles,
    weapon racks, anvil, grindstone, book stacks, candelabra/floor candlesticks,
    chains, cobwebs, mushrooms, lanterns, banners, rugs/carpets, statues,
    gravestones/coffins, cages, ladders, coin/treasure piles
  - Tabletop: candles & candlesticks, plates/bowls, mugs/tankards, bottles & flasks,
    books/scrolls, quill + inkpot, food (bread, cheese, apple), coin stacks, gems,
    potion vials, maps/parchment, dice, knives, small pouches

## UI

- [ ] Better item icons — current render-to-texture silhouettes are recognizable but rough
  (flat shading, no fill/depth, weapons-only). Options: tune the icon studio lighting/material for
  nicer 3D thumbnails, add proper icon meshes for all items (potions, gems, etc.), or hand-authored
  2D icon textures per item.

## Audio

- [ ] Ambient music (dungeon background track; later boss-fight / town variants)
