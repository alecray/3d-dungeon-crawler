> Guide generated 2026-06-08; verify against code as it evolves.

# Adding a Regular (Non-Boss) Enemy — End-to-End Guide

This guide lets another agent add a new regular enemy from zero to fully spawning, animated, and
rewarding XP — with the human only supplying art and animations.

---

## 1. Overview — How Regular Enemies Work

The system is **data-driven**: every regular enemy is defined by an `FMonsterDef` struct in
`MonsterDatabase::BuildTypes()` (`Source/DungeonCrawler/Enemies/MonsterTypes.cpp`). No subclass is
needed for a plain melee-chaser.

`ADungeonGenerator::ScatterMonsters()` (`Source/DungeonCrawler/Dungeon/DungeonGenerator.cpp`) populates
rooms by calling `MonsterDatabase::RollRandomType(Rng)` — a weighted random draw — and then calling
`Monster->ApplyType(TypeId)` on a freshly spawned `AMonsterCharacter`. `ApplyType` looks up the def
and calls `SetupSkeletalBody`, which synchronously loads the soft-object mesh + anims. If the mesh path
doesn't resolve (art not yet imported) the monster falls back to a **graybox cube body** and still
chases, attacks, and grants XP normally.

### When to subclass vs. when to add a data entry

| Situation | Approach |
|---|---|
| New melee chaser (same AI as Crab) | Add a `FMonsterDef` entry — **no C++ changes** |
| Ranged shooter, charger, exploder, or other custom AI | Subclass `AMonsterCharacter` (advanced, section 3) |

The Crab is the only fully-defined enemy today. Brute and Skitterer definitions are noted in code as
backlogged.

---

## 2. Add a Data-Driven Enemy (Common Case)

### 2a. Define the entry in `MonsterDatabase::BuildTypes()`

File: `Source/DungeonCrawler/Enemies/MonsterTypes.cpp`

Add a new block **inside** `BuildTypes()`, after the Crab block. Use the Crab as the template:

```cpp
// Skeleton — replace all "Skeleton" / "skeleton" labels with your enemy's name.
{
    FMonsterDef Skeleton;
    Skeleton.Id          = TEXT("Skeleton");       // unique FName key — used everywhere else
    Skeleton.MaxHealth   = 50.f;                   // HP
    Skeleton.MoveSpeed   = 300.f;                  // cm/s chase speed
    Skeleton.AttackDamage = 10.f;                  // damage per swing
    Skeleton.AttackRange  = 200.f;                 // cm from center; lunge anims need a bit more reach
    Skeleton.BodyScale   = 1.f;                    // graybox cube scale (1 = default humanoid size)
    Skeleton.MeshScale   = 1.f;                    // uniform scale on the imported skeletal mesh
    Skeleton.XPReward    = 25;                     // XP awarded on death
    Skeleton.CapsuleRadius     = 40.f;             // collision capsule (cm); auto-overridden by SetupSkeletalBody
    Skeleton.CapsuleHalfHeight = 88.f;             // collision capsule (cm); auto-overridden by SetupSkeletalBody

    // Asset soft references — point at the conventional content paths (see section 4).
    // These are TSoftObjectPtr, NOT raw path strings, so a moved/renamed asset is caught at cook time.
    Skeleton.SkeletalMeshPath = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/Enemies/Regular/Skeleton/SK_Skeleton.SK_Skeleton")));
    Skeleton.RunAnimPath  = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Enemies/Regular/Skeleton/A_Skeleton_Run.A_Skeleton_Run")));
    Skeleton.IdleAnimPath = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Enemies/Regular/Skeleton/A_Skeleton_Idle.A_Skeleton_Idle")));
    Skeleton.AttackAnimPath = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Enemies/Regular/Skeleton/A_Skeleton_Attack.A_Skeleton_Attack")));

    Skeleton.Weight = 100.f;   // spawn probability weight; Crab is also 100, so 50/50 split with two entries
    Types.Add(Skeleton);
}
```

**Key `FMonsterDef` fields** (all in `MonsterTypes.h`):

| Field | Type | Notes |
|---|---|---|
| `Id` | `FName` | Unique string key. Never collide with an existing Id. |
| `MaxHealth` | `float` | Starting/max HP. |
| `MoveSpeed` | `float` | `CharacterMovement->MaxWalkSpeed` (cm/s). |
| `AttackDamage` | `float` | Damage per swing. |
| `AttackRange` | `float` | Distance (cm, center-to-center) at which the monster swings. |
| `BodyScale` | `float` | Graybox cube scale. Unused once the skeletal mesh loads. |
| `MeshScale` | `float` | Uniform scale of the imported `USkeletalMeshComponent`. The capsule auto-fits. |
| `XPReward` | `int32` | XP granted to the player via `UStatsComponent::AddXP` on death. |
| `CapsuleRadius` | `float` | Initial capsule size. `SetupSkeletalBody` overrides this from mesh bounds if art is present. |
| `CapsuleHalfHeight` | `float` | Same. |
| `SkeletalMeshPath` | `TSoftObjectPtr<USkeletalMesh>` | Leave null/default → graybox. |
| `RunAnimPath` | `TSoftObjectPtr<UAnimSequence>` | Looping locomotion. Falls back to `IdleAnim` if missing. |
| `IdleAnimPath` | `TSoftObjectPtr<UAnimSequence>` | Looping idle. Falls back to `RunAnim` if missing. |
| `AttackAnimPath` | `TSoftObjectPtr<UAnimSequence>` | One-shot; plays each swing. |
| `Weight` | `float` | Weighted random probability. 0 = never spawns. |

### 2b. How the capsule auto-fits

When `SetupSkeletalBody` resolves the mesh it computes capsule size from the mesh's `GetBounds()` and
`MeshScale`. The values in `CapsuleRadius`/`CapsuleHalfHeight` are therefore only meaningful for the
graybox fallback — set them to a rough humanoid default (40 / 88) and let the art drive the real size.

### 2c. How spawn weighting works

`RollRandomType` does a weighted draw across all entries whose `Weight > 0`. With the Crab at `100.f`
and your new enemy also at `100.f`, each has a 50 % chance per spawn. Set the Crab to `200.f` and yours
to `100.f` for a 2:1 Crab bias. Set yours to `0.f` to disable it without deleting the entry.

### 2d. Graybox first

The new enemy works immediately after the code change — it spawns as the default hunched humanoid cube
body, chases, swings on the `AttackCooldown` (default 1.4 s), grants XP, and triggers hit-react pops.
Art can drop in later without touching this code again.

---

## 3. Add a Custom-Behaviour Enemy (Advanced)

Use this path only when the enemy needs AI that `AMonsterCharacter` does not provide out of the box —
for example a ranged shooter, a charging dasher, or an exploder. These archetypes are backlogged but the
extension points exist.

### 3a. Subclass `AMonsterCharacter`

```cpp
// MyRangedEnemy.h
#pragma once
#include "MonsterCharacter.h"
#include "MyRangedEnemy.generated.h"

UCLASS()
class DUNGEONCRAWLER_API AMyRangedEnemy : public AMonsterCharacter
{
    GENERATED_BODY()
public:
    AMyRangedEnemy();
    virtual void Tick(float DeltaSeconds) override;

protected:
    // Override this to intercept the chase phase (return true = skip the base navmesh chase).
    // See ABossMonster::TickCustomChase for a real example of the lunge/scuttle pattern.
    virtual bool TickCustomChase(float DeltaSeconds, APawn* Player,
                                 const FVector& DirToPlayer, float Dist) override;
};
```

Reference pattern: `ABossMonster` (`Source/DungeonCrawler/Enemies/BossMonster.h/.cpp`) overrides
`Tick`, `ApplyHitDamage`, and `TickCustomChase`. It calls `SetupSkeletalBody` manually in `BeginPlay`
and does NOT call `ApplyType` (it owns its own stat initialization). A simpler subclass can still call
`ApplyType` to let the database drive stats and mesh loading, then add behavior on top.

### 3b. Useful extension hooks in `AMonsterCharacter`

| Hook | Purpose |
|---|---|
| `virtual bool TickCustomChase(float, APawn*, const FVector&, float)` | Return `true` to replace the default navmesh chase this frame. |
| `virtual float ApplyHitDamage(float, const FVector&)` | Override to add weak points, resistances, etc. Call `Super` or manually route to `Health->ApplyDamage`. |
| `virtual bool IsBoss()` | Return `true` if you want this counted as a boss kill on death. |
| `void TriggerHitReact()` | Plays the body-scale pop without applying damage (for AoE or special-attack feedback). |
| `void SetBodyEmissive(float)` | Drives `M_Base`'s `EmissiveStrength` parameter — glow effects. |
| `bool SetupSkeletalBody(...)` | Call manually with Death + Flinch anim args if you want those optional clips. |
| `float AttackHitFrame` | Set > 0 to make the melee blow land mid-swing (dodgeable); the base is −1 (instant). |
| `float BodyScale` | Adjust in the constructor to resize the graybox body. |

### 3c. Register the subclass so it spawns

Option A — replace all regular monsters with your subclass by setting the generator's `MonsterClass`
property. In the Level Blueprint or a game mode, find `ADungeonGenerator` and set its `MonsterClass` to
your subclass. This swaps out every regular spawn.

Option B — spawn the subclass for a specific data entry by NOT using `ApplyType` and instead building
stat values in the subclass constructor from hard-coded values or a separate lookup. This is what
`ABossMonster` does.

There is currently no per-`FMonsterDef` class pointer field. If you need mixed spawning (some slots
spawn `AMyRangedEnemy`, others spawn `AMonsterCharacter`), the cleanest approach is to add a
`TSubclassOf<AMonsterCharacter> SpawnClass` field to `FMonsterDef` and read it in
`ScatterMonsters()` when calling `World->SpawnActor`.

---

## 4. Art and Animation Checklist (for the Human)

The human supplies the following assets exported from Blender as FBX:

### 4a. Assets needed

- [ ] **Skeletal mesh FBX** — the rigged character mesh (e.g. `SK_Skeleton.fbx`). Must have a
      **single root bone** (no stray root objects, "Add Leaf Bones" off in Blender FBX export settings).
- [ ] **Material instance / textures** — base color texture minimum (e.g. `T_Skeleton_BC`). The
      project uses `M_Base` as the master material; create a Material Instance that overrides the
      diffuse slot, or let the importer auto-generate one.
- [ ] **Idle animation FBX** — looping standing idle.
- [ ] **Run (or walk) animation FBX** — looping locomotion.
- [ ] **Attack animation FBX** — one-shot melee swing. The blow connects at frame ~0 by default
      (instant). If you want a dodgeable hit-frame, set `AttackHitFrame` in the subclass constructor.
- [ ] *(Optional)* **Death animation FBX** — one-shot; played by `HandleDeath` instead of the
      code sink/spin effect.
- [ ] *(Optional)* **Flinch/hit-react animation FBX** — one-shot; played on taking damage
      (skipped mid-swing so it doesn't cut an attack short).

### 4b. Conventional content paths

All regular-enemy assets live under `/Game/Enemies/Regular/<Name>/`. Mirror the Crab layout:

```
Content/Enemies/Regular/Skeleton/
    SK_Skeleton.uasset          ← skeletal mesh
    SKEL_Skeleton.uasset        ← skeleton asset (created on first mesh import)
    MI_Skeleton.uasset          ← material instance
    T_Skeleton_BC.uasset        ← base color texture
    A_Skeleton_Idle.uasset
    A_Skeleton_Run.uasset
    A_Skeleton_Attack.uasset
    A_Skeleton_Death.uasset     (optional)
    A_Skeleton_Flinch.uasset    (optional)
```

These paths must match the `FSoftObjectPath` strings you set in `MonsterDatabase::BuildTypes()`.

### 4c. Importing with `Tools/import_meshes.py`

**Step 1 — Import the skeletal mesh (with the editor open, in the Python Cmd bar):**

```python
import importlib, import_meshes; importlib.reload(import_meshes)
import_meshes.import_skeletal(
    r"E:\Game Projects\3d-dungeon-crawler\blends\enemies\skeleton.fbx",
    "/Game/Enemies/Regular/Skeleton"
)
```

This creates `SK_Skeleton`, `SKEL_Skeleton`, and auto-generated material assets in the content folder.
Note the skeleton asset path — you need it to import animations.

**Step 2 — Import each animation onto the existing skeleton:**

```python
import_meshes.import_anim(
    r"E:\Game Projects\3d-dungeon-crawler\blends\enemies\skeleton-idle.fbx",
    "/Game/Enemies/Regular/Skeleton",
    "/Game/Enemies/Regular/Skeleton/SKEL_Skeleton.SKEL_Skeleton",
    name="A_Skeleton_Idle"
)
```

Repeat for Run, Attack, Death, Flinch. The `name` parameter forces the output `AnimSequence` asset
name so it matches the soft-ref path in the database.

**Anim skeleton mismatch warning:** if the Output Log prints `[Monster X] Run/Walk anim is bound to a
DIFFERENT skeleton`, the anim FBX was exported from a different armature than the mesh FBX. Re-export
both from the same Blender armature, or use `import_anim` with the correct `SKEL_*` asset path.

---

## 5. Wire-Up Checklist and Testing

### Build and reload

- [ ] Save `MonsterTypes.cpp`.
- [ ] Build the editor target (PowerShell, with the editor **closed**):
  ```powershell
  & "Z:\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" `
      DungeonCrawlerEditor Win64 Development `
      -project="E:\Game Projects\3d-dungeon-crawler\DungeonCrawler.uproject" -waitmutex
  ```
- [ ] If the editor was open, use **Live Coding** (Ctrl+Alt+F11) to hot-reload the changed `.cpp`. If
      the compile fails or you changed a `.h`, do a full editor restart instead.

### In-editor testing

- [ ] Open the project in the editor, open any dungeon level, and **Play in Editor (PIE)**.
- [ ] The dungeon generates on `BeginPlay`; enter a non-Rest room and look for your new enemy.
- [ ] **Confirm it chases** — it should pursue you within `AggroRange` (default 1600 cm).
- [ ] **Confirm it attacks** — it swings when within `AttackRange`, damaging you on cooldown.
- [ ] **Confirm death** — kill it with melee; it should sink/spin (or play the death anim), poof, and
      disappear. Check the XP bar for the reward.
- [ ] **Confirm XP** — kill the enemy and watch the player's XP counter (HUD / stats screen).

### Verify the skeletal mesh loaded

If the mesh does not appear (graybox cube body persists after art import):

- Open the **Output Log** and search for the enemy's name — `SetupSkeletalBody`'s `LoadAnim` lambda
  prints a warning for any asset that fails to resolve.
- Double-check that the `FSoftObjectPath` strings in `MonsterTypes.cpp` match the actual asset paths in
  the Content Browser exactly (including the `AssetName.AssetName` suffix format).

### Tuning weights and stats

- Adjust `Weight` in `BuildTypes()` to dial spawn frequency relative to other enemies.
- Hot-reload after each tweak (or restart if the header changed) and re-enter PIE — the dungeon
  regenerates every play session, so no manual cleanup is needed.
- `dc.DebugOverlays 1` in the console draws the melee danger zone on the floor so you can verify
  `AttackRange` feels right.

---

## 6. Gotchas

### Soft-ref path format

The `FSoftObjectPath` string format is `"/Game/Path/To/Asset.AssetName"` — the asset name is repeated
after the dot. Omitting the dot-suffix causes the soft ref to resolve to null silently at runtime.
Example: `/Game/Enemies/Regular/Skeleton/SK_Skeleton.SK_Skeleton`.

### Headless import crashes (Interchange + multi-root)

UE 5.7 routes FBX import through Interchange, which asserts in a headless commandlet. Two workarounds:

1. **Always import in the editor** (drag-drop or the Python Cmd bar with `import_meshes.py`) — this is
   the recommended path and avoids the crash entirely.
2. If you must import headless, add
   `-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0` to the `UnrealEditor-Cmd`
   args to force the legacy importer. The legacy importer additionally **rejects multi-root rigs** —
   ensure the Blender armature has exactly one root bone (disable "Add Leaf Bones", remove stray root
   objects). See the README's "Headless animation-only import" section for a full runner example.

### Editor restart vs. live reload

- Changed only `.cpp` (no `.h` additions, no new UPROPERTYs): Ctrl+Alt+F11 live reload is enough.
- Changed a `.h`, added a UPROPERTY, or added a new class: **full editor restart** required.

### Regular enemies do not drop loot

`HandleDeath` in `MonsterCharacter.cpp` awards XP via `UStatsComponent::AddXP` and tallies
`GI->GetStats().MonstersKilled++`. It does **not** spawn any item pickup. If you want a regular enemy
to drop loot, add an `ItemPickup` spawn inside `HandleDeath` (or in an override in a subclass), using
the item-spawning pattern already present in `ALootChest`. Until then, only XP and the kill count are
recorded.

### `MonsterDatabase` uses static lazy-init

`BuildTypes()` is called once and the result is cached in a function-local `static`. Adding a new entry
and rebuilding is sufficient — there is no registration step. But because it is statically initialized,
**you cannot hot-patch the table at runtime**; a full rebuild + restart (or live reload) is required to
see a new entry.

### `MonsterClass` on `ADungeonGenerator`

The generator's `MonsterClass` property (visible in the Details panel) defaults to
`AMonsterCharacter::StaticClass()`. This is the class spawned for every regular slot. If you create a
subclass and want it to spawn, either set `MonsterClass` on the generator actor in the level, or set
it in the generator's constructor (`MonsterClass = AMyRangedEnemy::StaticClass()`).
