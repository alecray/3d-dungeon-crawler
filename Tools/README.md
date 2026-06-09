# Tools

Headless Python scripts for asset authoring and import pipelines. All scripts run via
`UnrealEditor-Cmd.exe` with the editor **closed** unless noted otherwise.

> **Path placeholders:** `<UE>` = your UE 5.7 install root (e.g. `Z:\Epic Games\UE_5.7`);
> `<PROJECT>` = this repository's root. Use **forward slashes** in `-script` paths —
> backslashes mangle escape sequences (`\3` gets eaten by the arg parser).

**Common invocation pattern:**

```
"<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<PROJECT>/DungeonCrawler.uproject" ^
    -run=pythonscript -script="<PROJECT>/tools/<script>.py" -unattended -nopause -nosplash
```

---

## `import_meshes.py`

Reusable FBX **import pipelines** so you never have to re-toggle import settings between
static and skeletal meshes again. Defines one locked-in preset for each type
(static / skeletal), plus single-file and batch helpers. Defaults suit this project:
Blender FBX exports (normals imported, not recomputed), Nanite **off** on static meshes,
materials + textures imported, auto-collision on statics, morph targets on skeletals,
optional skeleton reuse. All shared knobs live in one `CONFIG` block at the top.

**In-editor** (Output Log → Cmd bar set to *Python*):

```python
import importlib, import_meshes; importlib.reload(import_meshes)
import_meshes.import_static(r"<PROJECT>\blends\world\SM_Tree.fbx", "/Game/World")
import_meshes.import_skeletal(r"...\SK_Staff.fbx", "/Game/Weapons/Staff",
                              skeleton="/Game/Weapons/Staff/SKEL_Staff")  # reuse a skeleton (optional)
import_meshes.import_folder(r"...\blends\world", "/Game/World", kind="static")  # batch a folder
```

(For `import import_meshes` to resolve, add `tools/` under **Project Settings → Plugins →
Python → Additional Paths**, or run the full path once: `py "<PROJECT>/tools/import_meshes.py"`.)

**Headless** — use forward slashes, and note that this project's path contains a space
(`Game Projects`); for spaced paths call the functions from a small runner `.py` instead:

```
"<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<PROJECT>/DungeonCrawler.uproject" ^
    -run=pythonscript -script="<PROJECT>/tools/import_meshes.py static <PROJECT>/blends/.../SM_Foo.fbx /Game/World" ^
    -unattended -nopause -nosplash
```

`-script` args: `static <fbx> <dest>` · `skeletal <fbx> <dest> [skeletonAssetPath]` ·
`anim <fbx> <dest> <skeletonAssetPath> [name]` · `folder <srcDir> <dest> <static|skeletal>`.

**Headless animation-only import** (`import_anim` / the `anim` verb) — two gotchas:

- **Disable Interchange for the run.** UE 5.7 routes FBX import through Interchange, whose
  import path asserts in a headless commandlet. Force the legacy importer:
  add `"-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0"` to the
  `UnrealEditor-Cmd` args (and/or call
  `unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")`
  at the top of the script).
- **The rig must have a SINGLE root bone.** The legacy importer rejects multi-root
  hierarchies. If a headless anim import fails on this, re-export from Blender with one
  armature/root (disable "Add Leaf Bones"), or just import that one in the editor.

Example runner (handles the spaced path):

```python
import sys, unreal
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
sys.path.insert(0, r"<PROJECT>/tools"); import import_meshes
import_meshes.import_anim(r"<PROJECT>/blends/enemies/foo.fbx", "/Game/Enemies",
                          "/Game/Enemies/SKEL_Foo.SKEL_Foo", name="A_Foo_Death")
```

**Adding another pipeline:** add a builder returning a configured `unreal.FbxImportUI`,
mirroring `build_static_options()` / `build_skeletal_options()`; add a thin wrapper that
calls `_run(fbx, dest, your_options())`; put shared knobs in the `CONFIG` dict.

### Drag-drop presets (Interchange Pipeline Stacks)

The project also registers two custom **Interchange pipeline presets** so the Content
Browser import dialog's **"Pipeline Stack Preset"** dropdown offers **"Static Meshes"**
and **"Skeletal Meshes"** — each locked to its type with its own remembered settings.

- Preset assets: `/Game/Pipelines/PL_StaticMesh`, `/Game/Pipelines/PL_SkeletalMesh`.
  Built by **`make_interchange_presets.py`** (see below). Re-run if the assets are missing.
- Registration: `Config/DefaultEngine.ini` →
  `[/Script/InterchangeEngine.InterchangeProjectSettings]`
  `ContentImportSettings.PipelineStacks`. Editing this needs an **editor restart**.
- To add another preset: add a stack in that ini line pointing at a new
  `/Game/Pipelines/PL_*` asset, and create that asset in `make_interchange_presets.py`.

---

## `make_interchange_presets.py`

Builds the `/Game/Pipelines/PL_StaticMesh` and `/Game/Pipelines/PL_SkeletalMesh` pipeline
assets used by the Content Browser import dialog. Run headless with the editor **closed**
if the preset assets go missing (e.g. after a full content resync).

---

## `build_town.py`

Authors the town hub level (`/Game/Maps/Town/L_Town`) as placed, editable actors rather
than spawning them at runtime. Running it (re)builds the entire clearing: SkyAtmosphere +
atmosphere sun + SkyLight + height fog, the invisible boundary ring, the enclosing forest
(tree line + deeper trees), a distant vista ring, the hub stations (Shop / Blackjack /
Fishing / enter-dungeon Portal / Bonfire), and scatter scenery.

Run with the editor **closed**:

```
"<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<PROJECT>/DungeonCrawler.uproject" ^
    -run=pythonscript -script="<PROJECT>/tools/build_town.py" -unattended -nopause -nosplash
```

Key things to know:

- **One knob sizes everything:** `CLEARING_HALF` (cm, default `1100` ≈ 22 m square) drives
  the boundary, tree ring, station spacing, and scatter. Lower it to shrink the clearing,
  then re-run.
- **Everything is anchored to the `PlayerStart`** and filed under the World Outliner folder
  **`TownHub`**.
- **It's destructive-but-scoped:** each run deletes only actors already in the `TownHub`
  folder and respawns them — hand-placed actors outside that folder are untouched. **Manual
  edits to `TownHub` actors are lost on re-run.** Save `L_Town` after a run.
- **The boundary** is four invisible `BlockAll` boxes labeled **`Boundary1`–`Boundary4`**,
  hidden behind the tree line (trees have no collision). Toggle viewport collision view
  (Alt+C) to see them.

---

## `make_water_material.py`

Builds the fishing pond's water material **`/Game/World/M_PondWater`** entirely in code.
Rerun after editing the recipe.

```
"<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<PROJECT>/DungeonCrawler.uproject" ^
    -run=pythonscript -script="<PROJECT>/tools/make_water_material.py" -unattended -nopause -nosplash
```

- Run with the editor **closed** (deletes + recreates the asset). `AFishingHole`
  soft-loads `M_PondWater` at runtime so the script can freely rebuild it; the actor tints
  it per-instance via an MID (**Water Color** knob drives `ShallowColor`).
- Clarity-first stylized pond: depth-based shallow→deep color, low opacity + **refraction
  (IOR ~1.18)** so you see the bottom, a gentle smooth gradient-noise normal, a clamped
  Fresnel sky-rim, and a thin foam line. Everything is a material parameter —
  `RefractionIOR`, `ShallowOpacity`/`DeepOpacity`, `EdgeColor`,
  `RippleStrength`/`RippleScale`, `Roughness`.
- **Don't regress:** never drive the normal with `MaterialExpressionVectorNoise` (aliases
  into TV static). Use smooth `MaterialExpressionNoise` (Gradient) or a panned tiling
  normal map. Keep emissive sparkle off — locked-bright exposure clips it to white.

---

## `make_wisp_material.py`

Builds the Wisp enemy's **`/Game/Enemies/Regular/Wisp/M_Wisp`** master material and a
**`MI_Wisp`** instance, then assigns `MI_Wisp` to slot 0 on `SK_Wisp`.

The material is **Unlit + Translucent** with two parameters:
- `WispColor` (vector, default blue-purple `(0.35, 0.25, 1.0)`) — drives both emissive
  color and opacity alpha. Can be overridden per-instance for color variants.
- `EmissiveStrength` (scalar, default `5.0`) — multiplied by the color. Values > 1.0 push
  into HDR range, triggering bloom from the project's Post Process Volume automatically.

Run headless with the editor **closed** if `M_Wisp` or `MI_Wisp` are missing. Re-run any
time you change the material recipe.

---

## `make_card_material.py`

Builds the **`/Game/Cards/M_Card`** master material used by the blackjack table. It's
unlit, masked, and two-sided, with a single `CardTex` texture parameter. Card faces are
decoded from PNG at runtime in C++ and bound to this parameter via a dynamic material
instance — no texture import step is needed. Emissive is dimmed below the bloom threshold
so bright white card faces render flat/matte instead of glowing.

Run headless if `M_Card` goes missing.

---

## `place_blackjack.py`

Places an `ABlackjackTable` actor into `L_Town` near the player start (as a persistent
placed actor, not a runtime spawn) and saves the level. Skips placement if a table already
exists. Run once after initial setup; reposition the actor in-editor afterward.

---

## `add_emissive_to_mbase.py`

Adds a self-illumination parameter (`EmissiveStrength`, scalar, default 0) to
**`/Game/Furniture/M_Base`** so the boss and any other mesh using that material can glow
when driven above 0. Because the default is 0, all existing materials using `M_Base` are
visually unchanged. Idempotent — skips if `EmissiveStrength` already exists.

---

## `disable_nanite.py`

Turns Nanite **off** on every static mesh under `/Game`. This low-poly project doesn't
benefit from Nanite and it adds overhead (forces SM6 + Virtual Shadow Maps + dynamic-only
lighting). Run this to clear the "uses Nanite but Virtual Shadow Maps are not enabled"
warnings after importing new static meshes.

---

## Adding a new script

Follow the same headless invocation pattern as above. A minimal template:

```python
"""One-line description of what this script does and when to run it."""
import unreal
# ... your code
unreal.log("Done.")
```

Register any shared config (paths, toggles) at the top of the file.
