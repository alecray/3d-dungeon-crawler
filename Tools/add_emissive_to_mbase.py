"""Add a self-illumination param to M_Base: Emissive = BaseColorTex.RGB * EmissiveStrength (scalar,
default 0 so every existing material that uses M_Base is unchanged). Driven >0 on the boss to make it glow.
"""
import unreal

path = "/Game/Furniture/M_Base.M_Base"
mat = unreal.EditorAssetLibrary.load_asset(path)
mel = unreal.MaterialEditingLibrary

# Skip if already added.
if "EmissiveStrength" in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mat):
    unreal.log("M_Base already has EmissiveStrength — nothing to do")
else:
    # Re-sample the same BaseColorTex param (shares the texture binding by name) and scale it.
    tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 350)
    tex.set_editor_property("parameter_name", "BaseColorTex")

    strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 560)
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", 0.0)

    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 400)
    mel.connect_material_expressions(tex, "RGB", mul, "A")
    mel.connect_material_expressions(strength, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log("Added EmissiveStrength to M_Base")
