"""Headless: turn OFF Nanite on every Static Mesh under /Game (low-poly game doesn't want Nanite — it
adds overhead on light scenes and forces SM6 + Virtual Shadow Maps + dynamic-only lighting). Clears the
"uses Nanite but Virtual Shadow Maps are not enabled / SM6 required" warnings.

Run with the editor CLOSED:
  UnrealEditor-Cmd.exe <proj> -run=pythonscript -script="Tools/disable_nanite.py" -unattended -nopause -nosplash
"""
import unreal

EAL = unreal.EditorAssetLibrary
SES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

changed = 0
for path in EAL.list_assets("/Game", recursive=True, include_folder=False):
    asset = EAL.load_asset(path)
    if not isinstance(asset, unreal.StaticMesh):
        continue
    settings = asset.get_editor_property("nanite_settings")
    if not settings.get_editor_property("enabled"):
        continue
    settings.set_editor_property("enabled", False)
    try:
        SES.set_nanite_settings(asset, settings, apply_changes=True)  # sets + rebuilds the mesh
    except Exception:
        asset.set_editor_property("nanite_settings", settings)
    EAL.save_asset(path)
    changed += 1
    unreal.log("Disabled Nanite on {}".format(path))

unreal.log("disable_nanite: turned Nanite OFF on {} static mesh(es)".format(changed))
