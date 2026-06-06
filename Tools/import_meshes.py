"""Reusable FBX import pipelines so you never have to re-toggle import settings between static and
skeletal meshes again. Defines one locked-in preset for each type, plus single-file + batch helpers.

The project is low-poly (Nanite OFF) and the meshes come from Blender FBX exports, so the presets:
  - import normals from the FBX (preserve Blender's custom normals — no recompute)
  - keep Nanite disabled on static meshes (matches Tools/disable_nanite.py)
  - import materials + textures
  - static : combine meshes, auto-collision
  - skeletal: import morph targets, can reuse an existing skeleton, no physics asset by default

Tweak the CONFIG block once and both pipelines follow it.

────────────────────────────────────────────────────────────────────────────────────────────────
USAGE

In-editor (Output Log → Cmd bar, the little "Cmd"/"Python" dropdown set to Python):
    import importlib, import_meshes; importlib.reload(import_meshes)
    import_meshes.import_static(r"E:\Game Projects\3d-dungeon-crawler\blends\world\SM_Tree.fbx", "/Game/World")
    import_meshes.import_skeletal(r"...\SK_Staff.fbx", "/Game/Weapons/Staff",
                                  skeleton="/Game/Weapons/Staff/SKEL_Staff")  # reuse a skeleton (optional)
    import_meshes.import_folder(r"...\blends\world", "/Game/World", kind="static")

    (For the editor to find the module, this Tools/ folder must be on the Python path. Either run the
     full path once:  py "E:/Game Projects/3d-dungeon-crawler/Tools/import_meshes.py"  — or add Tools/
     under Project Settings → Plugins → Python → Additional Paths.)

Headless (editor CLOSED):
    UnrealEditor-Cmd.exe <proj.uproject> -run=pythonscript ^
        -script="Tools/import_meshes.py static \"E:/path/SM_Foo.fbx\" /Game/World" ^
        -unattended -nopause -nosplash
    ...or pass  skeletal <fbx> <dest> [skeletonAssetPath]  /  folder <srcDir> <dest> <static|skeletal>
────────────────────────────────────────────────────────────────────────────────────────────────
"""
import os
import sys
import unreal

# ── CONFIG ──────────────────────────────────────────────────────────────────────────────────────
CONFIG = {
    "import_materials": True,
    "import_textures": True,
    # static
    "combine_static_meshes": True,
    "auto_generate_collision": True,
    "generate_lightmap_uvs": True,
    "build_nanite": False,          # low-poly game: keep Nanite OFF
    # skeletal
    "import_morph_targets": True,
    "create_physics_asset": False,  # weapons/props don't need one; flip True for characters
    # shared (Blender FBX): import the FBX's normals rather than recomputing them
    "import_normals": True,
    "uniform_scale": 1.0,
}

_AT = unreal.AssetToolsHelpers.get_asset_tools()


def _set(obj, prop, value):
    """Set an editor property defensively — logs and skips if the property name differs in this build,
    instead of aborting the whole import."""
    try:
        obj.set_editor_property(prop, value)
    except Exception as exc:  # noqa: BLE001
        unreal.log_warning("import_meshes: skipped {}.{} = {!r} ({})".format(
            type(obj).__name__, prop, value, exc))


def _normal_method():
    return (unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
            if CONFIG["import_normals"]
            else unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)


# ── PIPELINES (preset option builders) ────────────────────────────────────────────────────────────
def build_static_options():
    """The Static Mesh import pipeline."""
    opts = unreal.FbxImportUI()
    _set(opts, "import_mesh", True)
    _set(opts, "import_as_skeletal", False)
    _set(opts, "import_animations", False)
    _set(opts, "import_materials", CONFIG["import_materials"])
    _set(opts, "import_textures", CONFIG["import_textures"])
    _set(opts, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    data = opts.get_editor_property("static_mesh_import_data")
    _set(data, "combine_meshes", CONFIG["combine_static_meshes"])
    _set(data, "auto_generate_collision", CONFIG["auto_generate_collision"])
    _set(data, "generate_lightmap_u_vs", CONFIG["generate_lightmap_uvs"])
    _set(data, "build_nanite", CONFIG["build_nanite"])
    _set(data, "normal_import_method", _normal_method())
    _set(data, "import_uniform_scale", CONFIG["uniform_scale"])
    return opts


def build_skeletal_options(skeleton=None):
    """The Skeletal Mesh import pipeline. Pass `skeleton` (asset path or USkeleton) to reuse an
    existing skeleton instead of creating a new one (e.g. re-importing an anim/mesh onto SKEL_Staff)."""
    opts = unreal.FbxImportUI()
    _set(opts, "import_mesh", True)
    _set(opts, "import_as_skeletal", True)
    _set(opts, "import_animations", True)
    _set(opts, "import_materials", CONFIG["import_materials"])
    _set(opts, "import_textures", CONFIG["import_textures"])
    _set(opts, "create_physics_asset", CONFIG["create_physics_asset"])
    _set(opts, "mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

    if skeleton is not None:
        skel = unreal.load_asset(skeleton) if isinstance(skeleton, str) else skeleton
        if skel:
            _set(opts, "skeleton", skel)
        else:
            unreal.log_warning("import_meshes: could not load skeleton {!r} — importing a new one".format(skeleton))

    data = opts.get_editor_property("skeletal_mesh_import_data")
    _set(data, "import_morph_targets", CONFIG["import_morph_targets"])
    _set(data, "normal_import_method", _normal_method())
    _set(data, "import_uniform_scale", CONFIG["uniform_scale"])
    return opts


# ── IMPORT RUNNERS ────────────────────────────────────────────────────────────────────────────────
def _run(fbx, dest, options):
    if not os.path.isfile(fbx):
        unreal.log_error("import_meshes: file not found: {}".format(fbx))
        return []
    task = unreal.AssetImportTask()
    _set(task, "filename", fbx)
    _set(task, "destination_path", dest)
    _set(task, "automated", True)        # no dialog
    _set(task, "replace_existing", True) # re-import in place
    _set(task, "save", True)             # save the new asset(s)
    _set(task, "options", options)
    _AT.import_asset_tasks([task])
    paths = list(task.get_editor_property("imported_object_paths") or [])
    for p in paths:
        unreal.log("import_meshes: imported {}".format(p))
    if not paths:
        unreal.log_warning("import_meshes: nothing imported from {} (check the FBX / settings)".format(fbx))
    return paths


def import_static(fbx, dest):
    """Import one FBX as a Static Mesh into `dest` (a /Game/... content path)."""
    return _run(fbx, dest, build_static_options())


def import_skeletal(fbx, dest, skeleton=None):
    """Import one FBX as a Skeletal Mesh into `dest`. Optional `skeleton` reuses an existing one."""
    return _run(fbx, dest, build_skeletal_options(skeleton))


def import_folder(src_dir, dest, kind="static", skeleton=None):
    """Batch-import every .fbx in `src_dir` (non-recursive) as `kind` = 'static' or 'skeletal'."""
    if not os.path.isdir(src_dir):
        unreal.log_error("import_meshes: folder not found: {}".format(src_dir))
        return []
    fbxs = sorted(f for f in os.listdir(src_dir) if f.lower().endswith(".fbx"))
    out = []
    for f in fbxs:
        full = os.path.join(src_dir, f)
        out += (import_skeletal(full, dest, skeleton) if kind == "skeletal" else import_static(full, dest))
    unreal.log("import_meshes: batch imported {} file(s) from {}".format(len(fbxs), src_dir))
    return out


# ── HEADLESS ENTRY (argv) ──────────────────────────────────────────────────────────────────────────
def _main(argv):
    if not argv:
        unreal.log("import_meshes: loaded. Call import_static / import_skeletal / import_folder.")
        return
    cmd = argv[0].lower()
    if cmd == "static" and len(argv) >= 3:
        import_static(argv[1], argv[2])
    elif cmd == "skeletal" and len(argv) >= 3:
        import_skeletal(argv[1], argv[2], argv[3] if len(argv) >= 4 else None)
    elif cmd == "folder" and len(argv) >= 4:
        import_folder(argv[1], argv[2], kind=argv[3], skeleton=argv[4] if len(argv) >= 5 else None)
    else:
        unreal.log_error("import_meshes: bad args {!r}. Use: static <fbx> <dest> | "
                         "skeletal <fbx> <dest> [skeleton] | folder <dir> <dest> <static|skeletal>".format(argv))


if __name__ == "__main__":
    # When run via -run=pythonscript, args after the script name land in sys.argv[1:].
    _main(sys.argv[1:])
