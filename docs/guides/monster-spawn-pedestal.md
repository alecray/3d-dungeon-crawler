# Monster Spawn Pedestal (enemy test station)

A small interactable pillar you can drop in town to spawn any enemy or boss on demand for testing. Walk
up to it, press **E**, and a grid menu opens with two tabs — **Enemies** and **Bosses**. Click an entry and
that creature appears in front of the pedestal as a **passive test dummy**: it wanders idly around the
spawn point and ignores you until you land the first hit, then it turns fully hostile (normal AI).

## Where it lives

| Piece | File |
| --- | --- |
| Pedestal actor | `Source/DungeonCrawler/Town/MonsterSpawnPedestal.{h,cpp}` |
| Spawn menu UI | `Source/DungeonCrawler/UI/MonsterSpawnWidget.{h,cpp}` |
| Passive roam behaviour | `AMonsterCharacter::SetPassive` / `TickPassiveRoam` (`Enemies/MonsterCharacter.cpp`) |
| Interaction wiring | `AFirstPersonCharacter::Interact` + `ADungeonPlayerController::OpenSpawnMenu` |
| Auto-placement in town | `ATownGameMode::EnsureSpawnPedestal` |

## Placing it

You don't have to do anything — `ATownGameMode::EnsureSpawnPedestal()` drops one a few metres to the side of
the PlayerStart on load **if the level doesn't already contain one**. To put it somewhere permanent, place an
`AMonsterSpawnPedestal` in `L_Town` in the editor (or via `Tools/build_town.py`); the code fallback then
steps aside.

## The pedestal model + animation

The pedestal is a skeletal mesh with a single animation, `A_Spawn_Pedestal`, that plays once each time a
monster is spawned. Until the FBX is imported it shows a graybox cylinder. Import to these paths (or repoint
the `EditAnywhere` soft refs on the actor):

- Mesh: `/Game/Town/SpawnPedestal/SK_Spawn_Pedestal`
- Animation: `/Game/Town/SpawnPedestal/A_Spawn_Pedestal`

(Skeletal/animation FBX imports must be done **in-editor** — headless Interchange import crashes on this
project; see the headless-FBX note in memory.)

## Passive → aggressive behaviour

`SetPassive(true)` makes a monster:
- ignore the player entirely (skips all chase/attack logic),
- wander to random points within `RoamRadius` of its spawn point, idling a beat at each, at 40% move speed,
- flip to normal aggressive AI the instant `HandleDamaged` fires from a player hit.

Bosses work too: `ABossMonster::Tick` honours `IsPassive()` and skips every boss special (slams, projectiles,
lunges) until engaged. Bosses are spawned at tier 0 and skip their intro freeze.

## Adding a boss to the menu

Enemies come straight from `MonsterDatabase::All()`, so any new entry there shows up automatically. Bosses
aren't in that registry, so add a row to `BossEntries()` in `MonsterSpawnWidget.cpp`:

```cpp
static TArray<FBossEntry> BossEntries()
{
    return {
        { TEXT("Hermit Crab"), ABossMonster::StaticClass() },
        // { TEXT("New Boss"), ANewBoss::StaticClass() },
    };
}
```

## Tuning knobs (Details panel on the pedestal)

- `SpawnForwardOffset` — how far in front dummies appear (cm).
- `RoamRadius` — how far they wander from the spawn point (cm).
- `PedestalMeshPath` / `SpawnAnimPath` — repoint the model/animation.

The menu's **Clear Spawned** button destroys every dummy that pedestal has spawned.

## Build note

Adding the pedestal/menu introduced **new C++ classes**, so Live Coding (Ctrl+Alt+F11) can't pick them up —
this required a full build with the editor closed. Subsequent tweaks to existing files hot-reload normally.
