"""Headless: build M_Stars — a stylized night-sky star material for the town day/night cycle.

Applied to a large inverted "star dome" sphere by ADayNightCycle, which fades it in at night via a
dynamic instance. Design goals:
  - UNLIT + ADDITIVE + TWO-SIDED, so when StarBrightness = 0 the dome is fully invisible (it NEVER
    occludes the SkyAtmosphere sky) and at night the stars just add glow over the dark sky.
  - Sparse star dots from a high-frequency noise raised to a high power (cheap, no textures), tinted a
    faint blue-white, scaled by the exposed scalar parameter **StarBrightness** (driven from C++).
Everything is parameterized; tune the look in-editor afterward. Defensive: node/pin names vary by
engine version, so creation + links are guarded — failures log "make_stars:" and leave a valid material
(at worst a uniform faint additive glow driven by StarBrightness, which the cycle still works with).

Run with the editor CLOSED:
  <UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe <PROJECT>/DungeonCrawler.uproject ^
      -run=pythonscript -script="<PROJECT>/Tools/make_stars_material.py" -unattended -nopause -nosplash
"""
import unreal

MAT_DIR, MAT_NAME = "/Game/World", "M_Stars"
mat_path = MAT_DIR + "/" + MAT_NAME
tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("make_stars: " + msg)


# Fresh material each run so the recipe is the source of truth.
if EAL.does_asset_exist(mat_path):
    EAL.delete_asset(mat_path)

mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
if not mat:
    raise Exception("make_stars: failed to create material")

# Core material properties: unlit, additive (transparent base), two-sided.
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("two_sided", True)


def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def connect(from_expr, from_out, to_expr, to_in):
    try:
        mel.connect_material_expressions(from_expr, from_out, to_expr, to_in)
        return True
    except Exception as e:
        log("connect failed ({} -> {}): {}".format(from_out, to_in, e))
        return False


# StarBrightness scalar (driven by the day/night actor: 0 by day, up at night).
brightness = expr(unreal.MaterialExpressionScalarParameter, -800, 200)
brightness.set_editor_property("parameter_name", "StarBrightness")
brightness.set_editor_property("default_value", 0.0)

# Faint blue-white star tint.
star_color = expr(unreal.MaterialExpressionConstant3Vector, -800, 360)
star_color.set_editor_property("constant", unreal.LinearColor(0.85, 0.9, 1.0, 1.0))

# Star dots: sample a high-frequency noise over the sphere UVs and raise it to a high power so only the
# brightest specks survive -> a sparse star field. Best-effort; if any node/pin is unavailable we fall
# back to a uniform glow (dots = 1).
dots = None
try:
    uv = expr(unreal.MaterialExpressionTextureCoordinate, -1200, -40)
    uv.set_editor_property("u_tiling", 22.0)  # lower density = sparser, softer stars (less sub-pixel shimmer)
    uv.set_editor_property("v_tiling", 22.0)

    uv3 = expr(unreal.MaterialExpressionAppendVector, -1000, -40)  # (u, v) -> (u, v, 0)
    zero = expr(unreal.MaterialExpressionConstant, -1200, 120)
    zero.set_editor_property("r", 0.0)
    connect(uv, "", uv3, "A")
    connect(zero, "", uv3, "B")

    noise = expr(unreal.MaterialExpressionNoise, -780, -40)
    try:
        noise.set_editor_property("scale", 1.0)
        noise.set_editor_property("function", unreal.NoiseFunction.NOISEFUNCTION_VORONOI)
        noise.set_editor_property("turbulence", False)
        noise.set_editor_property("levels", 1)
        noise.set_editor_property("output_min", 0.0)
        noise.set_editor_property("output_max", 1.0)
    except Exception as e:
        log("noise props partial: {}".format(e))
    connect(uv3, "", noise, "Position")

    sharpen = expr(unreal.MaterialExpressionPower, -560, -40)  # pow(noise, 7) -> softer specks (was 16: too sharp/sub-pixel -> shimmer)
    exp_c = expr(unreal.MaterialExpressionConstant, -780, 140)
    exp_c.set_editor_property("r", 7.0)
    connect(noise, "", sharpen, "Base")
    connect(exp_c, "", sharpen, "Exponent")
    dots = sharpen
except Exception as e:
    log("star-dot graph failed, falling back to uniform glow: {}".format(e))
    dots = None

# Emissive = dots * StarBrightness * color  (dots optional).
mul1 = expr(unreal.MaterialExpressionMultiply, -360, 120)
if dots is not None:
    connect(dots, "", mul1, "A")
    connect(brightness, "", mul1, "B")
else:
    connect(brightness, "", mul1, "A")  # no dots: uniform glow scaled by brightness
    connect(brightness, "", mul1, "B")  # (B unused-ish; kept simple)

mul2 = expr(unreal.MaterialExpressionMultiply, -180, 220)
connect(mul1, "", mul2, "A")
connect(star_color, "", mul2, "B")

connect(mul2, "", None, None) if False else None  # (no-op; explicit emissive connect below)
try:
    mel.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
except Exception as e:
    log("emissive connect failed: {}".format(e))

mel.recompile_material(mat)
EAL.save_asset(mat_path)
log("done -> " + mat_path)
