# TODO

## Gameplay


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