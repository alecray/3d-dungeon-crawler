# TODO

## Gameplay


- [x] Boss encounter sequence — boss spawns on room entry opposite the player; entrance doors seal for
      the fight (open on death); first-ever kill of a boss type plays a camera-focus + screen-shake +
      intro cinematic (persisted per boss id), replays just do the spawn animation.
- [x] Boss fight depth — crab-like scuttle + telegraphed lunge movement, player-targeting projectile
      volley (3-spread in phase 2+), phase-1 back weak point (2x dmg from behind, marked + auto-hidden
      after p1), and phase-2/3 bubble-pool hazards (ABubbleHazard). Reuses existing phase/special system.
- [x] Floating damage numbers — world-space 3D text on every hit to an enemy; weak-point hits show
      bigger/orange (ADamageNumber, billboards to camera, arcs up + shrinks out).
- [x] Improve enemy pathing (runtime navmesh + MoveTo so enemies path around walls instead of steering straight)
- [x] Dungeon traps — graybox spike floors (periodic), pressure plates (step-triggered spikes), and
      wall-mounted dart shooters. Scattered in corridors + occasional room cells via ADungeonTrap;
      mesh swap-in points (BaseMeshOverride/SpikeMeshOverride/DartMeshOverride + /Game/Traps paths).
- [x] Weapon usage costs stamina — melee now spends stamina (reducible by skills) like ranged/mage;
      the swing is gated when out of stamina. MeleeStaminaCost on the character.
- [x] Game juice pass — hit-stop on connecting melee, camera kick on melee hit + ranged fire
      (UHitCameraShake), enemy knockback on player hits, and a pulsing low-health red vignette.
- [ ] Trophy cases — placeable displays (e.g. in town) where players can show off items earned from the collection log
- [x] Make potions pick up on E press, not automatically (interact-to-collect instead of overlap auto-pickup)

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

- [ ] Status effects (poison, burn, freeze, stun, bleed) on weapons/abilities/enemies
- [ ] Loot rarity tiers + procedural affixes (rolled item stats: +damage, +crit, resistances, etc.)
- [ ] Multiple dungeon floors with descending, scaling difficulty (floor 1 → N)
- [ ] FTUE for controls — first-run onboarding surfacing movement/attack/interact/abilities/inventory.
- [ ] Dungeon-select menu on the town portal — lists dungeons/bosses you've cleared; replay at the same
      or harder difficulty tiers for greater reward potential, with the risk of losing everything in the
      run (high-stakes mode). Pairs with run-stakes / meta-progression.

## Boss — status & next

Done (graybox, all in C++, swaps to the hermit-crab model later):
- [x] Encounter flow: spawn-on-entry opposite the player, entrance doors seal (open on death),
      return portal activates on death.
- [x] Intro: camera focus + screen shake + rise/roar spawn animation on the FIRST kill of a boss id
      (persisted in save profile); replays play only the spawn animation.
- [x] 3 phases (health thresholds) with morph reveals + stat buffs.
- [x] Specials: ground slam, summon adds, enrage burst, projectile volley (3-spread p2+), bubble pools (p2-3).
- [x] Movement: crab scuttle + telegraphed lunge; hybrid navmesh (paths around cover when LoS blocked).
- [x] Phase-1 back weak point (2x damage from behind, marker hidden after p1).

Next / not yet done:
- [ ] VERIFY IN PIE: intro camera framing, ~1.8s hold length, doors line up + actually block, input
      hands back cleanly, boss scuttles/lunges without jitter at the LoS hand-off.
- [x] Shell-retreat mechanic (phase 3): tucks in = immobile + invulnerable (hits clang off) for
      ShellRetreatDuration, then re-emerges. In the phase-3 special pool.
- [ ] Pincer-sweep cone telegraph (replace/augment the radius-only slam).
- [x] Boss health bar UI — dedicated top-center red bar (UBossHealthBarWidget) shown for the fight,
      removed on death; labelled from the boss id.
- [x] Tuned projectile speed (boss bolts now 1500, dodgeable) + summoned adds get a real monster type.
- [ ] Swap graybox: hermit-crab model, real gate/portcullis door mesh, portal mesh.
- [ ] Support multiple boss types (only one boss id today).

### Future boss mechanic ideas (later)
- [ ] Burrow & resurface — boss digs under, re-emerges near the player (not for this boss).

## Flow / UX

- [ ] Controls in settings — a controls reference (and ideally rebinding) in the pause/settings menu.
- [x] Start screen — graybox main menu (title + Start/Quit) shown at launch over the boot map
      (opaque, once per session). Start fades in to play; Quit exits.
- [x] Scene transitions — black camera fades on every level travel (portal town<->dungeon, death
      restart) via DungeonPlayerController FadeToBlackAndTravel + fade-in on arrival.
  - Later: a dedicated main-menu level (needs an empty .umap authored in-editor; then point the
    GameMode/GameDefaultMap at it instead of overlaying the boot map).

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