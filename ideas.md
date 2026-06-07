# Ideas

> Loose brainstorm — concepts, wishlists, and jokes that aren't scoped tasks yet.
> When one firms up into actual work, move it into `TODO.md`.

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


prompt: 
I've added the fishing rod model, SK_Fishing_Rod. Please hook this up.
- The fishing rod should be required to "Cast" for a fish at the fishing pond
- Add the fishing rod as a purchaseable item from the shop
- Hook up the following animations:
  - "A_Fishing_Rod_Cast" (when casting a line out)
  - "A_Fishing_Rod_Idle" (when you have a line in and you're waiting for a fish to bite)
  - "A_Fishing_Rod_Reel_In" (when clicking to reel in a fish)
  
  animations. I haven't built these yet but that's what they will be called, so go ahead and connect them in code. They should trigger when you cast out to throw the bobber, and when you reel the bobber back in. 