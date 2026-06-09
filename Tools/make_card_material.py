"""Headless: build one unlit, masked, two-sided material (M_Card) with a 'CardTex'
texture parameter. Card textures are decoded from PNG at runtime in C++ and bound
to this parameter via a dynamic material instance, so no texture import is needed.
"""
import unreal

MAT_DIR = "/Game/Cards"
mat_path = MAT_DIR + "/M_Card"

tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_assets = unreal.EditorAssetLibrary

if editor_assets.does_asset_exist(mat_path):
    editor_assets.delete_asset(mat_path)

mat = tools.create_asset("M_Card", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property("two_sided", True)

mel = unreal.MaterialEditingLibrary
tex_param = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
tex_param.set_editor_property("parameter_name", "CardTex")

# Dim the emissive below the bloom threshold so bright white faces render flat/matte instead of
# glowing/glaring (unlit at full value blooms and looks "glossy" + washes out the pips).
dim = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 0)
dim.set_editor_property("const_b", 0.1)
mel.connect_material_expressions(tex_param, "RGB", dim, "A")

mel.connect_material_property(dim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(tex_param, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
mel.recompile_material(mat)
editor_assets.save_asset(mat_path)
unreal.log("Built master material M_Card at " + mat_path)
