# Ideas

> Loose brainstorm — concepts, wishlists, and jokes that aren't scoped tasks yet.
> When one firms up into actual work, move it into `TODO.md`.

improve boss death animation - fade away - more dark souls like - badass
if you hit the boss too many times in a row he should smack you backwards which stuns you for a sec
unstick button

todo:
- make the mini map better
  - make the player more visible
  - make the map easier to comprehend at a glance
  - make it look visually better
- improve the dungeon generation
  - add more connected hallways
- improve the scenery generation. research how.
- the wisp gets too close to the player, it should function the same as the other enemies as far as attacking is concerned
- boss should have a phase animation when it goes from 1->2
- ground items should have some kind of display on hover to show what kind of item they are and their stats. copy the implementation from borderlands for this, i like how its billboarded 3d.
- make sure we have "boss slain" text and a big XP drop
- create coin stacks and coin models
- high prio: leaving the dungeon shouldnt say "abandon run?" if you've killed the boss

## Gameplay / encounters
- Give one of the bosses a **receptionist** — maybe play elevator music when you walk inside, then
  you get told to take a seat.
- Boss idea: a **big clanker that eats tokens** LOL.
- magic find stat to increase chance of finding rare items

## Monsters to add
- eel
- manta ray / sting ray
- ant eater (boss)
- goat
- bear
- dog
- monkey
- snake
- squirrel

## Generation
- Add more variety to floor/wall generation.

## Models to make later
- sacks
- broken pottery shards
- hay piles
- candelabra / floor candlesticks
- lantern
- chains
- cobwebs
- rugs / carpets
- statues
- gravestones
- cage (wouldn't import; blend has been started)
- ladder
- coin / treasure piles


- redo skill tree
- redo items and consider fixed stats vs random stats on drops
  - item stats roll and land in a range
  - ilvl

Game loop:
- start/continue
- enter portal
- kill monsters for xp/gold/items
- find boss, kill boss
- each boss has a unique drop
- gear up character, level up skills
- felling a boss unlocks 1 or more other dungeon types / maps with included bosses
- felling a boss also unlocks the next upgraded variant of that boss
- end game: complete the collection log, slay all the bosses

- dynamic music would be a cool addition later

please write up a class/actor that is a small pillar that you can walk up to and press E to open a menu to choose a monster to spawn. i will create the fbx model, it will have one animation "A_Spawn_Pedestal this menu should be a grid of all available monsters and bosses, with filters for normal enemies and bosses. this spawned monster should not attack the player until the player attacks, and should just roam with the idle animation in a small area. once the player attacks, it should become aggressive. i want to place this actor in the town so i can test enemies. use up to 3 subagents for this task if necessary with sonnet. 