"""Build the Wisp's unlit emissive material (M_Wisp) and a matching material instance
(MI_Wisp), then assign MI_Wisp to slot 0 on SK_Wisp.

The material is Unlit + Masked (for bloom via HDR emissive without a lit shading model).
EmissiveStrength (scalar, default 5.0) × WispColor (vector, default blue-purple) drives
the emissive output. Values > 1.0 push into HDR and trigger bloom from the Post Process
Volume automatically.

Run with the editor CLOSED (deletes + recreates M_Wisp / MI_Wisp):
  UnrealEditor-Cmd.exe <proj> -run=pythonscript -script="<proj>/tools/make_wisp_material.py"
      -unattended -nopause -nosplash
"""
import unreal

CONTENT_DIR  = "/Game/Enemies/Regular/Wisp"
MAT_PATH     = CONTENT_DIR + "/M_Wisp"
INST_PATH    = CONTENT_DIR + "/MI_Wisp"
MESH_PATH    = CONTENT_DIR + "/SK_Wisp"

EAL   = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel   = unreal.MaterialEditingLibrary

# ---------------------------------------------------------------------------
# 1. Master material — M_Wisp
# ---------------------------------------------------------------------------

if EAL.does_asset_exist(MAT_PATH):
    EAL.delete_asset(MAT_PATH)

mat = tools.create_asset("M_Wisp", CONTENT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
# Translucent so the mesh reads as a soft glowing orb rather than a hard cutout.
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("two_sided", True)

# WispColor — vector parameter, default blue-purple (matches AWispMonster::WispColor).
color_param = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
color_param.set_editor_property("parameter_name", "WispColor")
color_param.set_editor_property("default_value", unreal.LinearColor(r=0.35, g=0.25, b=1.0, a=1.0))

# EmissiveStrength — scalar parameter, default 5.0 (HDR; triggers bloom automatically).
strength_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 200)
strength_param.set_editor_property("parameter_name", "EmissiveStrength")
strength_param.set_editor_property("default_value", 5.0)

# Multiply color × strength → emissive.
mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 80)
mel.connect_material_expressions(color_param, "RGB", mul, "A")
mel.connect_material_expressions(strength_param, "",   mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Opacity: use the color's alpha channel so the mesh can fade at the edges via vertex color
# or a fresnel mask later. Default alpha=1 → fully opaque.
mel.connect_material_property(color_param, "A", unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
EAL.save_asset(MAT_PATH)
unreal.log("Built M_Wisp at " + MAT_PATH)

# ---------------------------------------------------------------------------
# 2. Material instance — MI_Wisp
# ---------------------------------------------------------------------------

if EAL.does_asset_exist(INST_PATH):
    EAL.delete_asset(INST_PATH)

inst_factory = unreal.MaterialInstanceConstantFactoryNew()
inst_factory.set_editor_property("initial_parent", mat)
inst = tools.create_asset("MI_Wisp", CONTENT_DIR, unreal.MaterialInstanceConstant, inst_factory)
EAL.save_asset(INST_PATH)
unreal.log("Built MI_Wisp at " + INST_PATH)

# ---------------------------------------------------------------------------
# 3. Assign MI_Wisp to SK_Wisp slot 0
# ---------------------------------------------------------------------------

mesh = EAL.load_asset(MESH_PATH)
if mesh and isinstance(mesh, unreal.SkeletalMesh):
    skel_mat = unreal.SkeletalMaterial()
    skel_mat.set_editor_property("material_interface", inst)
    mesh.set_editor_property("materials", [skel_mat])
    EAL.save_asset(MESH_PATH)
    unreal.log("Assigned MI_Wisp to SK_Wisp slot 0")
else:
    unreal.log_warning(
        "SK_Wisp not found at {} — assign MI_Wisp manually in the editor.".format(MESH_PATH))

unreal.log("make_wisp_material: done.")
