"""Headless: create custom Interchange pipeline PRESETS so the Import dialog's "Pipeline Stack Preset"
dropdown offers per-type presets ("Static Meshes", "Skeletal Meshes"), each with its OWN remembered
import settings — so you stop re-tweaking the dialog when switching between static and skeletal assets.

Builds two pipeline assets under /Game/Pipelines by duplicating the engine's DefaultAssetsPipeline and
locking each to a mesh type via CommonMeshesProperties.ForceAllMeshAsType. The pipeline assets are then
registered as named stacks in Config/DefaultEngine.ini under
[/Script/InterchangeEngine.InterchangeProjectSettings] ContentImportSettings.PipelineStacks
(see README "Importing meshes"). Interchange remembers tweaked properties per stack, so each preset keeps
its own collision/material/etc. settings.

Run with the editor CLOSED:
  <UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe <PROJECT>/DungeonCrawler.uproject ^
      -run=pythonscript -script="<PROJECT>/Tools/make_interchange_presets.py" -unattended -nopause -nosplash
"""
import unreal

EAL = unreal.EditorAssetLibrary
SRC = "/Interchange/Pipelines/DefaultAssetsPipeline.DefaultAssetsPipeline"  # engine template to clone
DIR = "/Game/Pipelines"


def setp(o, p, v):
    if o is None:
        return False
    try:
        o.set_editor_property(p, v)
        return True
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("make_pipelines: set {} failed: {}".format(p, e))
        return False


def force_type(want):
    """Resolve an EInterchangeForceMeshType value (e.g. want='StaticMesh') across Python naming variants
    (the member is exposed as IFMT_STATIC_MESH). Matches by normalizing away underscores/case."""
    E = getattr(unreal, "InterchangeForceMeshType", None)
    if E is None:
        return None
    targets = {("ifmt" + want).replace("_", "").lower(), want.replace("_", "").lower()}
    for n in dir(E):
        if n.replace("_", "").lower() in targets:
            v = getattr(E, n, None)
            if v is not None:
                return v
    return None


def get_prop(o, name):
    try:
        return o.get_editor_property(name)
    except Exception:  # noqa: BLE001
        return None


def make_preset(dst_name, want, want_static):
    dst = DIR + "/" + dst_name
    if EAL.does_asset_exist(dst):
        EAL.delete_asset(dst)

    # A fresh generic-assets pipeline (same class as the engine default). We don't clone the engine
    # DefaultAssetsPipeline because it isn't in the asset registry under a headless commandlet.
    try:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        pipe = tools.create_asset(dst_name, DIR, unreal.InterchangeGenericAssetsPipeline, None)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("make_pipelines: could not create {}: {}".format(dst, e))
        return None
    if pipe is None:
        unreal.log_warning("make_pipelines: {} is null".format(dst))
        return None

    # Lock the preset to one mesh type so it always imports as static / skeletal.
    common = get_prop(pipe, "common_meshes_properties")
    ft = force_type(want)
    if common is not None and ft is not None:
        setp(common, "force_all_mesh_as_type", ft)
    else:
        unreal.log_warning("make_pipelines: ForceAllMeshAsType not set on {} (common={}, enum={})".format(dst, common is not None, ft))

    mp = get_prop(pipe, "mesh_pipeline")
    if mp is not None:
        setp(mp, "import_static_meshes", want_static)
        setp(mp, "import_skeletal_meshes", not want_static)

    EAL.save_loaded_asset(pipe)
    unreal.log("make_pipelines: built {} (force={}, applied={})".format(dst, want, ft is not None))
    return pipe


if not EAL.does_directory_exist(DIR):
    EAL.make_directory(DIR)

make_preset("PL_StaticMesh",   "StaticMesh",   True)
make_preset("PL_SkeletalMesh", "SkeletalMesh", False)
unreal.log("make_pipelines: done")
