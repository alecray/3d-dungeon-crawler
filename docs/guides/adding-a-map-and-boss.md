> Guide generated 2026-06-08; verify against code as it evolves.

# Adding a New Dungeon Map + Boss (End-to-End)

This guide is precise enough for another agent to implement a brand-new dungeon map and boss from scratch. The human only needs to supply art assets and animations — all wiring is C++.

---

## 1. Overview — The Moving Parts

```
Town L_Town
  └─ APortal (TargetMapName = "L_YourNewMap")
       └─ [E] → AFirstPersonCharacter::Interact
               → ADungeonPlayerController::OpenMapSelectMenu
               → UMapSelectWidget::Init(TargetMap)   ← shows map name + tier 0-3 selector
               → [Go] → GI->SetSelectedMap / SetSelectedTier
                       → FadeToBlackAndTravel("L_YourNewMap")

L_YourNewMap   (game mode = AYourDungeonGameMode, subclass of ADungeonCrawlerGameMode)
  └─ ADungeonCrawlerGameMode::BuildWorld   [BeginPlay]
       └─ spawns ADungeonGenerator (or your subclass)
            └─ ADungeonGenerator::Generate  [BeginPlay]
                 ├─ PlaceRooms / PlaceBossRoom / CarveCorridors / BuildGeometry
                 └─ SetupBossEncounter
                      └─ spawns ABossArena::Configure(BossClass, RoomCenter, RoomHalf, PortalClass, TownMapName)
                           └─ [player enters trigger] → StartEncounter
                                ├─ spawns ABossMonster subclass
                                ├─ Boss->SetTier( GI->GetSelectedTier() )
                                ├─ Boss->PlayIntro()  →  ABossSpawnVFX + intro camera + UBossIntroCameraShake
                                ├─ ABossDoor::Seal() on each doorway
                                └─ [boss dies] → ABossDoor::Open, loot drop, APortal activated back to L_Town
```

Key files:

| What | File |
|---|---|
| Game mode (dungeon) | `Source/DungeonCrawler/Core/DungeonCrawlerGameMode.{h,cpp}` |
| Game mode (town) | `Source/DungeonCrawler/Core/TownGameMode.{h,cpp}` |
| Dungeon generator | `Source/DungeonCrawler/Dungeon/DungeonGenerator.{h,cpp}` |
| Boss base class | `Source/DungeonCrawler/Enemies/BossMonster.{h,cpp}` |
| Boss arena | `Source/DungeonCrawler/Enemies/BossArena.{h,cpp}` |
| Boss door | `Source/DungeonCrawler/Enemies/BossDoor.{h,cpp}` |
| Boss health bar | `Source/DungeonCrawler/UI/BossHealthBarWidget.{h,cpp}` |
| Portal | `Source/DungeonCrawler/Dungeon/Portal.{h,cpp}` |
| Map-select UI | `Source/DungeonCrawler/UI/MapSelectWidget.{h,cpp}` |
| Game instance (tier/map selection) | `Source/DungeonCrawler/Core/DungeonGameInstance.{h,cpp}` |
| Spawn VFX | `Source/DungeonCrawler/VFX/BossSpawnVFX.{h,cpp}` |
| Attack telegraph | `Source/DungeonCrawler/VFX/AttackTelegraph.{h,cpp}` |

---

## 2. Adding a New Map

### 2a. Create the Level Asset

1. In the editor, **File → New Level → Empty Level**. Save it as `/Game/Maps/Dungeons/L_YourNewMap`.
2. No content needed in the level — the game mode builds everything at runtime. An empty level works fine (see the existing `L_DungeonTest` pattern).

### 2b. Create a Game Mode for the New Map

The simplest path: subclass `ADungeonCrawlerGameMode`. If the new map uses the exact same generator settings as the existing dungeon you can optionally skip this and reuse `ADungeonCrawlerGameMode` directly, but a dedicated subclass lets you customise the generator class later.

**`Source/DungeonCrawler/Core/YourMapGameMode.h`** (new file):
```cpp
#pragma once
#include "DungeonCrawlerGameMode.h"
#include "YourMapGameMode.generated.h"

UCLASS()
class DUNGEONCRAWLER_API AYourMapGameMode : public ADungeonCrawlerGameMode
{
    GENERATED_BODY()
public:
    AYourMapGameMode();
};
```

**`Source/DungeonCrawler/Core/YourMapGameMode.cpp`** (new file):
```cpp
#include "YourMapGameMode.h"
#include "YourDungeonGenerator.h"  // your custom generator subclass if you made one

AYourMapGameMode::AYourMapGameMode()
{
    // Optionally override the generator class here:
    // DungeonGeneratorClass = AYourDungeonGenerator::StaticClass();
}
```

`DungeonCrawlerGameMode::BuildWorld` (the inherited `BeginPlay` handler) does all of: `EnsureLighting`, `EnsurePostProcess`, `EnsureFog`, spawns the `DungeonGeneratorClass`, and drops the player into room 0.

### 2c. Assign the Game Mode to the Level

1. Open `/Game/Maps/Dungeons/L_YourNewMap` in the editor.
2. **World Settings → GameMode Override** → pick `AYourMapGameMode`.
3. Save the level.

Without a game-mode override the engine falls back to the project default; explicitly setting it ensures the right generator is spawned.

### 2d. Add the Map to the Town Portal

The portal in `L_Town` determines what `UMapSelectWidget` shows. Its `TargetMapName` field (an `FName` set in `APortal::TargetMapName`, editable via the Details panel in `L_Town`) drives the whole flow:

1. Run `Tools/build_town.py` headless (or edit the portal actor in `L_Town` in the editor) and set the **enter-dungeon portal's `TargetMapName`** to `L_YourNewMap`.
2. The portal passes its `TargetMapName` to `OpenMapSelectMenu` → `UMapSelectWidget::Init(TargetMap)` which displays it in the Destination label. No C++ changes needed for a single-destination portal.

If you want **multiple destinations** (one portal per dungeon, or a multi-entry widget), the current `UMapSelectWidget` shows a single destination passed via `Init`. Extending it to a list requires adding a `TArray<FName>` to `Init` and generating one `MapEntry` border per destination — follow the existing `MapEntry` construction in `MapSelectWidget.cpp:Initialize()`.

### 2e. Wire the Generator to Your New Boss

The `ADungeonGenerator` has two `EditAnywhere` fields (set either in a subclass constructor or via the Details panel on the generator actor):

```cpp
// DungeonGenerator.h – already declared:
TSubclassOf<ABossMonster> BossClass;   // Category: "Dungeon|Boss"
TSubclassOf<ABossArena>   ArenaClass;  // Category: "Dungeon|Boss"
```

In your custom generator subclass constructor (or `YourMapGameMode` if you override `BuildWorld` and configure the generator after spawn):

```cpp
BossClass = AYourBoss::StaticClass();
// ArenaClass can stay as ABossArena::StaticClass() unless you need a custom arena
```

The generator's `SetupBossEncounter()` reads `BossClass` and passes it to `ABossArena::Configure`. The arena then spawns it via `World->SpawnActor<ABossMonster>(BossClass, ...)` and calls `Boss->SetTier(GI->GetSelectedTier())`.

The `TownMapName` field on the generator (default `"L_Town"`) controls where the post-boss return portal takes the player. If your dungeon should return somewhere else, set it in your generator subclass constructor:

```cpp
TownMapName = TEXT("L_Town"); // already the default; change if needed
```

---

## 3. Adding a New Boss

### 3a. Subclass vs. Parameterize

**Subclass `ABossMonster`** when the boss needs:
- A different movement pattern (override `TickCustomChase`)
- New special attacks beyond ground slam
- Different hit-damage routing (override `ApplyHitDamage`)
- A unique `BossId` (for intro-seen tracking in `UDungeonGameInstance::MarkBossIntroSeen`)

**Parameterize the existing `ABossMonster`** (set `EditAnywhere` fields in the Details panel or a constructor) when the boss is architecturally the same hermit-crab pattern but uses different art/stats.

For a meaningfully new boss, subclass:

### 3b. New Boss Header

**`Source/DungeonCrawler/Enemies/YourBoss.h`**:
```cpp
#pragma once
#include "BossMonster.h"
#include "YourBoss.generated.h"

/**
 * YourBoss — [brief description].
 * Tier 0: 2 phases. Phase 1 = [describe]. Phase 2 = [special unlocked].
 */
UCLASS()
class DUNGEONCRAWLER_API AYourBoss : public ABossMonster
{
    GENERATED_BODY()

public:
    AYourBoss();

    /** Override if you need a different weak-point / damage route. */
    virtual float ApplyHitDamage(float BaseDamage, const FVector& FromLocation) override;

protected:
    virtual void BeginPlay() override;

    /** Override for a unique movement pattern (return true to take over movement). */
    virtual bool TickCustomChase(float DeltaSeconds, APawn* Player,
        const FVector& DirToPlayer, float Dist) override;

    // ---- Skeletal mesh + anims ----
    // Follow the TSoftObjectPtr<T> convention (see README "Code conventions").
    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<USkeletalMesh> SkeletalMeshPath =
        TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/SK_YourBoss.SK_YourBoss")));

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    float SkeletalMeshScale = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<UAnimSequence> IdleAnimPath =
        TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/A_YourBoss_Idle.A_YourBoss_Idle")));

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<UAnimSequence> RunAnimPath =
        TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/A_YourBoss_Walk.A_YourBoss_Walk")));

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<UAnimSequence> AttackAnimPath =
        TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/A_YourBoss_Attack.A_YourBoss_Attack")));

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<UAnimSequence> SpawnAnimPath =
        TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/A_YourBoss_Spawn.A_YourBoss_Spawn")));

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<UAnimSequence> DeathAnimPath =
        TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/A_YourBoss_Death.A_YourBoss_Death")));

    UPROPERTY(EditAnywhere, Category = "Boss|Mesh")
    TSoftObjectPtr<UAnimSequence> FlinchAnimPath =
        TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
            TEXT("/Game/Enemies/Bosses/YourBoss/A_YourBoss_Flinch.A_YourBoss_Flinch")));

    // ---- Your new special attack ----
    UPROPERTY(EditAnywhere, Category = "Boss|YourAttack")
    float YourAttackDamage = 25.f;

    UPROPERTY(EditAnywhere, Category = "Boss|YourAttack")
    float YourAttackRadius = 400.f;

    UPROPERTY(EditAnywhere, Category = "Boss|YourAttack")
    float YourAttackWindup = 1.5f;

private:
    void YourSpecialAttack();
};
```

### 3c. New Boss Implementation

**`Source/DungeonCrawler/Enemies/YourBoss.cpp`**:
```cpp
#include "YourBoss.h"
#include "AttackTelegraph.h"
#include "HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AYourBoss::AYourBoss()
{
    // --- Identity ---
    BossId = TEXT("YourBoss");        // unique across all boss types; drives intro-seen save flag

    // --- Base stats (tier 0; SetTier scales from these) ---
    MonsterMaxHealth = 400.f;
    AttackDamage     = 110.f;
    AttackRange      = 320.f;
    AggroRange       = 6000.f;
    AttackCooldown   = 5.0f;
    AttackHitFrame   = 20.f;         // blow connects on frame 20 — dodgeable
    MoveSpeed        = 380.f;
    XPReward         = 350;

    // Save originals so SetTier can scale from them (not re-scale an already-scaled value).
    BaseMonsterMaxHealth = MonsterMaxHealth;
    BaseAttackDamage     = AttackDamage;

    IntroDuration = 2.0f;   // fallback if SpawnAnim is missing

    GetCapsuleComponent()->InitCapsuleSize(70.f, 140.f);
    GetCharacterMovement()->bOrientRotationToMovement = false; // always face the player
}

void AYourBoss::BeginPlay()
{
    Super::BeginPlay();   // calls SetupSkeletalBody with the parent's fields — override if yours differ

    // If your boss overrides the mesh/anim fields (UPROPERTY shadowing), call SetupSkeletalBody again:
    SetupSkeletalBody(SkeletalMeshPath, SkeletalMeshScale,
        RunAnimPath, IdleAnimPath, AttackAnimPath, DeathAnimPath, FlinchAnimPath);

    SetBodyEmissive(0.3f);
}

float AYourBoss::ApplyHitDamage(float BaseDamage, const FVector& FromLocation)
{
    // Add custom weak-point logic here, then delegate to the base for phase-life interception:
    return Super::ApplyHitDamage(BaseDamage, FromLocation);
}

bool AYourBoss::TickCustomChase(float DeltaSeconds, APawn* Player,
    const FVector& DirToPlayer, float Dist)
{
    // Return false to use the default navmesh chase.
    // Return true + call AddMovementInput to take full control of movement (see BossMonster.cpp).
    return false;
}

// ---- Your special attack: telegraph disc → deferred hit ----
void AYourBoss::YourSpecialAttack()
{
    UWorld* World = GetWorld();
    if (!World) { return; }

    const FVector Center = GetActorLocation();
    const float FloorZ = Center.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector TeleLoc(Center.X, Center.Y, FloorZ);

    FActorSpawnParameters P;
    P.Owner = this;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Spawn the red danger disc (auto-destroys after Windup + 0.18 s).
    if (AAttackTelegraph* Tel = World->SpawnActor<AAttackTelegraph>(
            AAttackTelegraph::StaticClass(), FTransform(TeleLoc), P))
    {
        Tel->Configure(YourAttackRadius, YourAttackWindup);
    }

    TriggerHitReact(); // visual wind-up tell on the boss body

    // Schedule the detonation with a timer (match the windup).
    FTimerHandle Handle;
    World->GetTimerManager().SetTimer(Handle, [this]()
    {
        if (!IsValid(this) || !Health || Health->IsDead()) { return; }
        if (APawn* Player = GetWorld()->GetFirstPlayerController()
                ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
        {
            FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
            ToPlayer.Z = 0.f;
            if (ToPlayer.SizeSquared() <= YourAttackRadius * YourAttackRadius)
            {
                if (UHealthComponent* PH = Player->FindComponentByClass<UHealthComponent>())
                {
                    PH->ApplyDamage(YourAttackDamage);
                }
            }
        }
    }, YourAttackWindup, /*bLoop*/ false);
}
```

**Note on `DoRandomSpecial` and phase gating:** `ABossMonster::DoRandomSpecial` is a `private` method. To add your special to the pool, you must override it. Either make it `protected` in `BossMonster.h` (a one-line edit) or reproduce the scheduler pattern in your subclass by overriding `Tick` and managing a `NextSpecialTime`. The simplest path: change `DoRandomSpecial` from `private` to `protected virtual` in `BossMonster.h`, then override it in `YourBoss`:

```cpp
// In YourBoss.h (protected):
virtual void DoRandomSpecial() override;   // requires DoRandomSpecial to be protected virtual in BossMonster

// In YourBoss.cpp:
void AYourBoss::DoRandomSpecial()
{
    if (GetTier() == 0 && GetCurrentPhase() >= 2)
    {
        YourSpecialAttack();
    }
    // Optionally call Super::DoRandomSpecial() for the base attacks too.
}
```

### 3d. Tier Scaling

`SetTier(int32 InTier)` is called by `ABossArena::StartEncounter` immediately after spawn. It:
- Clamps `InTier` to 0..3.
- Scales `MonsterMaxHealth` and `AttackDamage` by `(1 + TierStatScale * Tier)` from the saved base values.
- Sets `MaxPhases` = `(Tier == 0) ? 2 : (2 + Tier)` — tier 0 = 2 lives, tier 1 = 3, tier 2 = 4, tier 3 = 5.
- Writes the new `MaxHealth` onto `UHealthComponent` (already exists at this point in the arena flow).

The `TierStatScale` multiplier defaults to `0.5f` (`EditAnywhere` in `BossMonster.h`). Override in your subclass constructor if a different scaling curve is needed.

Unique tier 1–3 abilities are deferred (`TODO(tier1+)` in `BossMonster.cpp`). The infrastructure (phase pool, `EnrageEndTime`, `bShellRetreat`, `BubbleClass`, `ProjectileClass`) is already built — you just need to fill in art/content and un-gate them in `DoRandomSpecial`.

### 3e. Phase Advance and Per-Phase Unlocks

`ABossMonster::AdvanceToPhase(int32 NewPhase)` is called:
- On `BeginPlay` with `NewPhase = 1`.
- Each time the boss's HP hits 0 while `PhasesRemaining > 1` (the `ApplyHitDamage` multi-life intercept). HP is clamped to 1, `PhasesRemaining` is decremented, and phase is advanced before refilling.

Each `AdvanceToPhase` call:
- Reveals one `MorphPart` cube (`MorphParts[NewPhase - 1]`).
- Activates/deactivates the `BackWeakMesh` (only visible in phase 1).
- Buffs `AttackDamage *= 1.25`, `MoveSpeed += 40`, `AttackCooldown *= 0.85` for phases 2+.
- Calls `TriggerHitReact()` for a visual pop.

To unlock a special in phase 2 only, gate it in `DoRandomSpecial` with `if (GetCurrentPhase() >= 2)`. Add a public accessor `int32 GetCurrentPhase() const { return CurrentPhase; }` to `BossMonster.h` (currently `CurrentPhase` is private).

### 3f. Boss Health Bar Name + Star Indicator

`UBossHealthBarWidget::SetBoss(ABossMonster* Boss)` (called by `ABossArena::StartEncounter`) sets the name label and refreshes the star display. The **name text** is currently derived from the boss class name by default. To give your boss a readable name, override a method or add an `FText BossDisplayName` field to `ABossMonster` and read it in `BossHealthBarWidget::SetBoss`.

The **phase star row** (`PhaseText`) is rebuilt by `RefreshPhaseStars()` every tick:
- `★` per remaining life, `☆` per used life.
- Hidden when `MaxPhases <= 1`.
- The widget polls `Boss->GetPhasesRemaining()` and `Boss->GetMaxPhases()` each frame.

No code changes needed in the widget — just ensure your boss subclass correctly inherits `GetPhasesRemaining()` / `GetMaxPhases()` (both are public non-virtual on `ABossMonster`).

### 3g. Intro Cinematic + VFX

`ABossArena::StartEncounter` calls `Boss->PlayIntro()` and `RunIntroCinematic(Player)`:

1. **`PlayIntro()`** — loads `SpawnAnimPath` synchronously, plays it on the skeletal mesh component. Falls back to a procedural body-scale "pop" if the asset is missing. Sets `IntroDuration` from the clip length. Spawns `ABossSpawnVFX::Configure(IntroDuration, FLinearColor(...))` at floor level. The VFX color defaults to orange — set the color in your subclass constructor via `ABossSpawnVFX::Configure`'s second argument (see `PlayIntro` in `BossMonster.cpp` line 563–565 for where to change the tint if you override `PlayIntro`).

2. **`RunIntroCinematic`** — spawns a `ACameraActor` aimed at the boss, blends the player view to it (cubic ease-in/out, 0.6 s blend), plays `UBossIntroCameraShake`, holds for `IntroDuration + IntroEndBeat` (default 0.8 s), then blends back. No code changes needed; the camera positions itself based on the boss capsule half-height automatically.

### 3h. Arena Hookup (Summary)

You do not need to modify `ABossArena`. All you pass to it is in the generator's `SetupBossEncounter`:

```cpp
// DungeonGenerator.cpp – SetupBossEncounter()
Arena->Configure(BossClass, RoomCenter, RoomHalf, PClass, TownMapName);
```

`BossClass` is your `TSubclassOf<ABossMonster>`, which you set in the generator constructor or game mode. The rest is automatic.

---

## 4. Art and Animation Assets — What the Human Must Supply

### 4a. Skeletal Mesh + Materials/Textures

For `/Game/Enemies/Bosses/YourBoss/`:

| Asset | Content Browser path (default from TSoftObjectPtr) |
|---|---|
| Skeletal mesh | `SK_YourBoss.SK_YourBoss` |
| Physics asset | `PA_YourBoss` (auto-created on import) |
| Skeleton | `SKEL_YourBoss.SKEL_YourBoss` |
| Base colour texture | `T_YourBoss_BC` |
| Material instance | `MI_YourBoss` |

Path convention: `/Game/Enemies/Bosses/YourBoss/`. The `TSoftObjectPtr` defaults in the header must match exactly (asset name appears twice: `SK_Foo.SK_Foo`, matching the UE `<AssetName>.<AssetName>` format for soft object paths).

### 4b. Animation Clips Required

All clips must be imported onto the **same skeleton** (`SKEL_YourBoss`). Minimum set:

| Clip | Purpose | Notes |
|---|---|---|
| `A_YourBoss_Idle` | Standing idle loop | Looped; plays when boss is in range but not chasing |
| `A_YourBoss_Walk` | Chase / movement loop | Looped; plays while moving toward player (`RunAnimPath`) |
| `A_YourBoss_Attack` | Melee swing | One-shot; `AttackHitFrame` sets which frame the damage lands |
| `A_YourBoss_Spawn` | Intro / spawn-in | One-shot; drives `IntroDuration` (use its full length) |
| `A_YourBoss_Death` | Death | One-shot; replaces the code sink/spin effect |
| `A_YourBoss_Flinch` | Hit-react | One-shot; played on taking damage; optional but recommended |

Additional clips you may add for new specials (not wired by default — load and play them manually in your special-attack functions):

| Clip | Purpose |
|---|---|
| `A_YourBoss_Slam` | Ground slam wind-up |
| `A_YourBoss_ShellRetreat` | Shell tuck-in (if using shell mechanic) |
| `A_YourBoss_Roar` | Enrage animation |

### 4c. Blender → FBX → UE Import Pipeline

**Blender export settings (FBX):**
- One armature root bone — no stray root objects, "Add Leaf Bones" off. The legacy UE importer rejects multi-root hierarchies (`Multiple roots are found in the bone hierarchy`).
- Apply scale before export: `File → Export → FBX → Geometry → Apply Scalings = FBX Units Scale`.
- Export the mesh and armature together for the skeletal mesh FBX; export each animation separately as FBX with "Baked Animation" checked, selecting only the armature.

**Import via `Tools/import_meshes.py` (in-editor, Python console):**

```python
import importlib, import_meshes; importlib.reload(import_meshes)
# 1. Import the skeletal mesh (creates SK_YourBoss + SKEL_YourBoss):
import_meshes.import_skeletal(
    r"E:/Game Projects/3d-dungeon-crawler/blends/enemies/your-boss.fbx",
    "/Game/Enemies/Bosses/YourBoss")

# 2. Import each animation onto the existing skeleton:
import_meshes.import_skeletal(
    r".../A_YourBoss_Idle.fbx",
    "/Game/Enemies/Bosses/YourBoss",
    skeleton="/Game/Enemies/Bosses/YourBoss/SKEL_YourBoss")
# Repeat for Walk, Attack, Spawn, Death, Flinch.
```

To add the `Tools/` path so `import import_meshes` resolves: **Project Settings → Plugins → Python → Additional Paths** → add the absolute path to `Tools/`.

**Headless anim import (editor closed):** Requires two extra steps:
1. Force legacy FBX importer (Interchange asserts headless): add `"-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0"` to the `UnrealEditor-Cmd` args.
2. The rig must have a single root bone (see above).
3. Paths in `-script` args must use **forward slashes** — backslashes get mangled (`\A` etc.).
4. If the project path has spaces (`Game Projects`), use a small runner `.py` file instead of inline args. See the README "Headless animation-only import" section for the exact pattern.

**In-editor drag-drop:** Drag the FBX into the Content Browser, choose the **"Skeletal Meshes"** pipeline preset from the "Pipeline Stack Preset" dropdown (set up by `Tools/make_interchange_presets.py`). For subsequent animation clips, right-click the existing skeleton → **Reimport** or drag the FBX and select the existing skeleton in the dialog.

---

## 5. Wire-Up Checklist + How to Test

### Build Checklist

- [ ] New header(s) added to the right subfolder (`Enemies/` for boss, `Core/` for game mode).
- [ ] New `.cpp` files `#include` the header by bare filename (all subfolders are include paths in `DungeonCrawler.Build.cs`).
- [ ] `BossId` is unique (different from `"HermitCrab"` and any other existing bosses).
- [ ] `TSoftObjectPtr` default paths in the header match the actual content browser paths exactly (including the doubled asset name: `SK_YourBoss.SK_YourBoss`).
- [ ] Generator constructor sets `BossClass = AYourBoss::StaticClass()`.
- [ ] Level asset `/Game/Maps/Dungeons/L_YourNewMap` has `GameMode Override` set to your game mode class.
- [ ] Town portal `TargetMapName` = `L_YourNewMap`.
- [ ] Build passes: run Build.bat (see README "Running") via PowerShell — **never Bash** (spaces in engine path fail silently under Bash).

### Editor Restart Requirement

Any time you add a new `UPROPERTY`, `UCLASS`, or `UFUNCTION` (or change their signatures), you **must fully restart the editor** — not just Ctrl+Alt+F11 Live Coding reload. Live Coding only reloads function bodies.

### Test Sequence

1. **Build** with the editor closed. Open the editor. No compilation errors.
2. **PIE from town (L_Town):** Walk to the enter-dungeon portal, press **E**. The `UMapSelectWidget` should show your map name in the Destination label. Select Tier 0, click **Go**. Verify the fade-to-black and level transition.
3. **In the dungeon (`L_YourNewMap`):** The generator should spawn. Check the **minimap** (M or the minimap overlay) — it should show rooms + corridors with the boss room distinguished. Check the developer console for the boss-tier debug message: `Boss tier 0: HP=... ATK=... MaxPhases=2`.
4. **Walk to the boss room trigger:** The arena trigger fires. Doors seal. The intro cinematic plays (camera pans to boss, screen shake). The boss spawns on the far side of the room from you, facing you.
5. **Boss health bar:** The top-of-screen bar appears with the boss name and `★★` stars (2 phases, tier 0).
6. **Phase transition:** Damage the boss to 0 HP. It should refill and show `★` (1 remaining). The `GEngine->AddOnScreenDebugMessage` prints `THE BOSS ENTERS PHASE 2!`. The back weak point (visible bright square on the back) disappears. In phase 2, the ground slam fires (red disc, `GROUND SLAM in 1.8s — JUMP!`).
7. **Boss death:** Kill the boss in its final phase. Doors open. 3–5 loot items scatter. Return portal activates and takes you back to `L_Town`.
8. **Tier scaling:** Repeat with Tier 1 selected. Debug message should show `HP=...` at 1.5× baseline, `MaxPhases=3`.
9. **Art check:** If the skeletal mesh is imported, the graybox cubes should be hidden and the rigged mesh visible. If any `TSoftObjectPtr` path is wrong, the graybox fallback remains visible (not an error, just a missing-asset symptom).

---

## 6. Gotchas

### Soft-Ref Paths

`TSoftObjectPtr` default paths must use the doubled UE asset name format: `/Game/Enemies/Bosses/YourBoss/SK_YourBoss.SK_YourBoss` (asset name appears twice, separated by `.`). One wrong character and `LoadSynchronous()` returns null, the boss silently falls back to graybox cubes. Verify by checking the reference in the Content Browser: right-click the asset → **Copy Reference** and compare.

### Headless Import Pitfalls

- **Interchange crashes headless**: Always pass `-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0` when importing FBX from `UnrealEditor-Cmd`.
- **Multi-root rigs**: The legacy importer rejects them. Fix in Blender: ensure one armature, disable "Add Leaf Bones", and parent everything to a single root.
- **Spaced project path**: `E:\Game Projects\` has a space. Pass `-script` args as a runner `.py` file, not inline args (headless argv splits on spaces).
- **Forward slashes in all paths**: Backslashes in `-script` paths are mangled by the Windows shell before reaching Python. Use `/` everywhere in path strings passed to the command line.

### Editor Restart for New UPROPERTYs

Adding any `UPROPERTY`, `UCLASS`, or `UFUNCTION` — or changing their specifiers — requires a **full editor restart**, not Live Coding. Live Coding only patches function bodies. If the new field doesn't appear in the Details panel, you need a restart.

### Deferred Tier 1+ Ability Content

Tier 1–3 stat scaling already works. Unique abilities (`SpitProjectiles`, `BubbleBurst`, `SummonAdds`, `EnrageBurst`, `EnterShellRetreat`) are compiled into `ABossMonster` but gated behind `TODO(tier1+)` in `DoRandomSpecial`. They will fire if you un-gate them, but their content classes (`ProjectileClass`, `BubbleClass`) are set to base defaults with no art. Design and enable them once tier 1+ content is ready; until then, tier 1–3 fights are purely stat-scaled.

### `DoRandomSpecial` Visibility

`DoRandomSpecial` is currently `private` in `ABossMonster`. To override the ability pool in a subclass you need to either:
- Change it to `protected virtual` in `BossMonster.h` (one-line edit, then rebuild).
- Or manage your own special-scheduling timer in your subclass `Tick` alongside a call to `Super::Tick` (which already runs `DoRandomSpecial` via `TickCustomChase`).

### Phase Buff Compounding

`AdvanceToPhase` multiplies `AttackDamage *= 1.25` and increments `MoveSpeed` in place. Across 3–5 phases (tiers 2–3), these compound. The code comment (`BossMonster.cpp`) calls this out: for multi-phase tier 1+ designs, recompute per-phase from the tier-scaled base rather than multiplying in place, or the later phases will be disproportionately powerful.

### `CurrentPhase` is Private

`ABossMonster::CurrentPhase` is `private`. Add `int32 GetCurrentPhase() const { return CurrentPhase; }` to the `public:` section of `BossMonster.h` if your subclass needs to gate abilities on phase number.

### LFS for Binary Assets

Content Browser `.uasset` files, Blender `.blend` files, and FBX exports are tracked with Git LFS. Before adding new art files to the repo, confirm LFS is active:

```powershell
git lfs status
```

If it's not tracking the new content folder, add the appropriate patterns to `.gitattributes` (the existing patterns for `*.uasset`, `*.blend`, `*.fbx` should already cover it).
