> Guide generated 2026-06-09; verify against code as it evolves.

# Adding a New Weapon — End-to-End Guide

This guide covers adding a new melee weapon from zero to fully equipped, animated (including optional L/R
swing pairs), sold in the shop, and set as a class default — with the human only supplying the art and
animation FBX files.

The Club is the worked example throughout; substitute your weapon name wherever "Club" appears.

---

## 1. Overview — How Weapons Work

Weapons are **items** in the `ItemDatabase` (`Source/DungeonCrawler/Items/ItemTypes.cpp`). Each weapon
item has:

- An `EEquipKind` that drives `ECombatStyle` selection (`Sword` → Melee, `Crossbow` → Ranged,
  `Staff` → Mage).
- An optional **custom skeletal mesh** (`WeaponMeshPath`) shown in the player's hand.
- Optional **swing animation(s)** (`WeaponSwingAnimPath` / `WeaponSwingAnimAltPath`) played on the
  held mesh.

When the player equips a weapon via the hotbar, `AFirstPersonCharacter::EquipActiveHotbarItem()`
(`Source/DungeonCrawler/Player/FirstPersonCharacter.cpp`) swaps the held mesh and calls
`UCombatComponent::SetMeleeAnims()` to configure the swing animations.

### Weapon kinds and combat styles

| `EEquipKind` | `ECombatStyle` | What it enables |
|---|---|---|
| `Sword` | Melee | Melee swings + Whirlwind ability |
| `Crossbow` | Ranged | Bolt projectiles + Volley ability |
| `Staff` | Mage | Spell bolts + Nova ability |

All melee weapons use `EEquipKind::Sword` regardless of shape (swords, clubs, axes, etc.). The per-item
mesh and animation overrides distinguish them visually.

---

## 2. Add the Item Definition

File: `Source/DungeonCrawler/Items/ItemTypes.cpp`, inside `ItemDatabase::BuildItems()`.

### 2a. Add the `MakeItem` call

Find the weapons block and add your new entry:

```cpp
Items.Add(MakeItem(TEXT("Club"), TEXT("Club"), EItemType::Weapon, EItemRarity::Common, 1, 35, EEquipKind::Sword));
```

`MakeItem` signature: `(Id, DisplayName, ItemType, Rarity, MaxStack, GoldValue, EquipKind)`.

### 2b. Add a description

In the `SetDesc` lambda section:

```cpp
SetDesc(TEXT("Club"), TEXT("A weighted wooden club. Alternates left and right swings."));
```

### 2c. Add an icon (skeletal mesh)

Weapons use their held skeletal mesh as the inventory icon. In the `SetSkelIcon` lambda section:

```cpp
SetSkelIcon(TEXT("Club"), TEXT("/Game/Weapons/Club/SK_Club.SK_Club"));
```

---

## 3. Add Custom Mesh and Animations

Still in `ItemDatabase::BuildItems()`, after the `SetPotionIcon` calls, there are two lambdas
(`SetWeaponMesh`, `SetWeaponAnims`) for per-weapon overrides.

### 3a. Custom held mesh

If the weapon needs its own mesh (not the shared sword model):

```cpp
SetWeaponMesh(TEXT("Club"), TEXT("/Game/Weapons/Club/SK_Club.SK_Club"));
```

When `WeaponMeshPath` is set, `EquipActiveHotbarItem` loads it synchronously and assigns it to the held
mesh. When unset, it falls back to the default `SwordSkeletalAsset`.

### 3b. Single swing animation (most melee weapons)

For a weapon with one swing animation, set only `WeaponSwingAnimPath`. When equipped, the combat
component plays this instead of the default sword swing:

```cpp
SetWeaponAnims(TEXT("MyAxe"),
    TEXT("/Game/Weapons/MyAxe/A_MyAxe_Attack.A_MyAxe_Attack"),
    TEXT("")   // no alt → single animation, random L/R flinch on enemies
);
```

Actually for a single animation, just leave out the `SetWeaponAnims` call entirely — the default
`SwingAnim` on `UCombatComponent` (`/Game/Weapons/Sword/A_Sword_Attack`) will play and enemy flinch
direction is randomized.

### 3c. L/R alternating animations (Club pattern)

For a weapon with dedicated left and right swing animations, set both paths:

```cpp
SetWeaponAnims(TEXT("Club"),
    TEXT("/Game/Weapons/Club/A_Club_Attack_L.A_Club_Attack_L"),
    TEXT("/Game/Weapons/Club/A_Club_Attack_R.A_Club_Attack_R"));
```

When both are set:
- `UCombatComponent` alternates between `SwingAnim` (primary = Attack_L) and `SwingAnimAlt`
  (alt = Attack_R) on each swing.
- `UCombatComponent::DoMeleeHit` calls `Monster->SetPendingFlinchSide(bLastSwingWasLeft)` before
  applying damage — enemies play the matching `A_*_Flinch_L` or `A_*_Flinch_R` instead of a random one.

---

## 4. Add to the Shop

File: `Source/DungeonCrawler/Town/ShopNPC.cpp`, in `GetWares()`.

Add the weapon's `FName` id to the `DefaultStock` array:

```cpp
DefaultStock = {
    FName(TEXT("HealthPotion")), ...,
    FName(TEXT("Sword")), FName(TEXT("Club")), FName(TEXT("Crossbow")), ...,
};
```

The shop displays items in this order. Place the weapon near other weapons of the same style.

---

## 5. Set as a Class Default Weapon (Optional)

File: `Source/DungeonCrawler/Player/CharacterClass.cpp`, in `GetClassLoadout()`.

Each `ECharacterClass` case sets `L.WeaponId` to the item that fills hotbar slot 0 on class selection:

```cpp
case ECharacterClass::Warrior:
default:
    L.WeaponId = FName(TEXT("Club")); // was "Sword"
    break;
```

The loadout is applied in `AFirstPersonCharacter::ApplyClassLoadout()`, which writes the weapon to hotbar
slot 0 and calls `EquipActiveHotbarItem()`.

---

## 6. Art and Animation Checklist (for the Human)

### Assets needed

- [ ] **Skeletal mesh FBX** — the weapon model (e.g. `SK_Club.fbx`). Single-root skeleton if
      animations share the same armature. The mesh should be modeled at the held position and scale for
      the player's hand (camera space).
- [ ] **Material instance / textures** — base color texture minimum. The project uses `M_Base` as the
      master; create an MI that overrides the diffuse, or let the importer auto-generate one.
- [ ] **Primary swing animation FBX** — one-shot swing animation. For a single-animation weapon this is
      the only one needed; it replaces the default sword swing.
- [ ] *(L/R pair)* **Attack_L FBX + Attack_R FBX** — left-side and right-side swing variants. Needed
      only for weapons that use the directional flinch system (see section 3c).
- [ ] *(Optional)* **Deflect animation FBX** — played when the swing clips a wall instead of an enemy.
      Uses `DeflectAnim` on `UCombatComponent`; currently shared across all melee weapons.

### Conventional content paths

All weapon assets live under `/Game/Weapons/<Name>/`:

```
Content/Weapons/Club/
    SK_Club.uasset          ← skeletal mesh
    SKEL_Club.uasset        ← skeleton asset (created on first mesh import)
    MI_Club.uasset          ← material instance
    T_Club_BC.uasset        ← base color texture
    A_Club_Attack_L.uasset  ← left swing (optional; for L/R pair weapons)
    A_Club_Attack_R.uasset  ← right swing (optional)
```

These paths must match the `FSoftObjectPath` strings you set in `ItemDatabase::BuildItems()`.

### Importing with `tools/import_meshes.py`

**Step 1 — Import the skeletal mesh (with the editor open, in the Python Cmd bar):**

```python
import importlib, import_meshes; importlib.reload(import_meshes)
import_meshes.import_skeletal(
    r"E:\Game Projects\3d-dungeon-crawler\blends\weapons\club.fbx",
    "/Game/Weapons/Club"
)
```

Note the generated `SKEL_Club` asset path — you need it for animation imports.

**Step 2 — Import each animation onto the existing skeleton:**

```python
import_meshes.import_anim(
    r"E:\Game Projects\3d-dungeon-crawler\blends\weapons\club-attack-l.fbx",
    "/Game/Weapons/Club",
    "/Game/Weapons/Club/SKEL_Club.SKEL_Club",
    name="A_Club_Attack_L"
)
import_meshes.import_anim(
    r"E:\Game Projects\3d-dungeon-crawler\blends\weapons\club-attack-r.fbx",
    "/Game/Weapons/Club",
    "/Game/Weapons/Club/SKEL_Club.SKEL_Club",
    name="A_Club_Attack_R"
)
```

The `name` parameter forces the output `AnimSequence` asset name to match the soft-ref path in
`ItemDatabase::BuildItems()`.

---

## 7. Build and Test Checklist

### Build

- [ ] Save all modified `.cpp` / `.h` files.
- [ ] Build the editor target (PowerShell, with the editor **closed**):
  ```powershell
  & "Z:\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" `
      DungeonCrawlerEditor Win64 Development `
      -project="E:\Game Projects\3d-dungeon-crawler\DungeonCrawler.uproject" -waitmutex
  ```
- [ ] If the editor was open and only `.cpp` changed, **Ctrl+Alt+F11** to live-reload. If a `.h` changed
      or a new UPROPERTY was added, do a full editor restart.

### In-editor testing

- [ ] Open the project, go to the Town level, and **Play in Editor (PIE)**.
- [ ] Open inventory (I) → drag the Club to hotbar slot 1 and select it.
- [ ] **Confirm the mesh swaps** — the held weapon in view should change to the Club model.
- [ ] **Confirm the swing animation plays** — attack; the Club animation should show on the held mesh.
- [ ] **For L/R weapons**: swing twice — the first should play Attack_L, the second Attack_R (alternating).
- [ ] **Confirm enemy flinch** — hit an enemy; it should play the L flinch on Attack_L hits and R flinch
      on Attack_R hits (inspect `A_*_Flinch_L` / `A_*_Flinch_R` visually).
- [ ] **Confirm damage** — enemies should take damage and display damage numbers.
- [ ] **Confirm shop** — visit the Shop NPC and verify the Club appears in the item list at the correct
      price (35 gold for the Club).
- [ ] **Confirm class loadout** — if the weapon is a class default, start a new character of that class
      and confirm the weapon is pre-equipped in hotbar slot 1.

---

## 8. Gotchas

### `WeaponMeshPath` needs a loaded asset

`EquipActiveHotbarItem` calls `WeaponMeshPath.LoadSynchronous()` at equip time. If the mesh FBX has not
been imported yet, `LoadSynchronous` returns null and the held mesh hides itself (same as the graybox
fallback). Import the mesh before testing equip.

### `SwingAnimAlt` must be non-null for L/R flinch to trigger

`DoMeleeHit` only calls `Monster->SetPendingFlinchSide()` when `SwingAnimAlt.Get()` is non-null. If the
alt animation is not yet imported, the system silently falls back to random flinch — the weapon still
works, it just won't drive enemy flinch direction until the animations land.

### Animation FBX must share the weapon's skeleton

Import animations using the same `SKEL_*` asset the mesh import created. Using a mismatched skeleton
causes a silent retarget failure; the held mesh freezes in its bind pose during swings.

### Soft-ref path format

`FSoftObjectPath` strings must be in the format `"/Game/Path/Asset.AssetName"` — the asset name is
repeated after the dot. Example: `/Game/Weapons/Club/SK_Club.SK_Club`. A missing dot-suffix resolves to
null silently at runtime.

### Shop `DefaultStock` is a lazy-init static

`ShopNPC.cpp`'s `DefaultStock` is populated once inside an `if (DefaultStock.Num() == 0)` guard. Adding
a new item only takes effect after a full editor restart or Live Code reload.

### `ItemDatabase` uses a static lazy-init cache

Like `MonsterDatabase`, `ItemDatabase::BuildItems()` is cached in a function-local `static`. Adding a
new item requires a rebuild + restart (or live reload) before it is visible at runtime.
