"""Headless: build M_PondWater — a visible, stylized TRANSLUCENT water material for the fishing pond.

(Switched from Single Layer Water, which rendered near-black/clear over the dark basin.) CLARITY-FIRST calm stylized pond — no plugins, all built-in nodes. The look comes from what you can SEE
THROUGH the water (refraction + low opacity), not from piled-on surface detail:
  - Two-sided + translucent, so it shows regardless of the water quad's normal direction.
  - Depth-based color: shallow tint near the shoreline -> deeper tint over depth (SceneDepth - PixelDepth),
    with a gentle ramp so deep water still reads clear.
  - SMOOTH gentle ripples: two low-frequency, slow-panning MaterialExpressionNoise (GRADIENT, mip-filtered)
    fields tilt the normal in X/Y. Deliberately NOT VectorNoise — that cellular per-pixel noise aliased
    into TV static. Calm wavelength, low strength.
  - REFRACTION (Index Of Refraction ~1.18) bends the visible pond bottom — the main "crystal clear" cue.
  - Low, depth-driven OPACITY (clear at the shoreline, semi-clear deep) so you see the basin floor.
  - Soft, CLAMPED Fresnel sky-rim (never blows to white) + glossy-not-mirror roughness for a controlled glint.
  - A thin shoreline FOAM line with only a faint glow. NO emissive sparkle (it caused the blown whites).
Everything is parameterized for live tuning. Also sets SM_Fishing_Hole to complex-as-simple collision.

Defensive: node/pin names vary by version, so creation + links are guarded — failures log "make_water:"
and leave a still-valid material.

Run with the editor CLOSED:
  <UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe <proj> -run=pythonscript ^
      -script="<PROJECT>/Tools/make_water_material.py" -unattended -nopause -nosplash
"""
import unreal

MAT_DIR, MAT_NAME = "/Game/World", "M_PondWater"
mat_path = MAT_DIR + "/" + MAT_NAME
tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def setp(o, p, v):
    if o is None:
        return False
    try:
        o.set_editor_property(p, v); return True
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("make_water: set {}.{} failed: {}".format(type(o).__name__, p, e)); return False


def node(cls_name, x, y):
    cls = getattr(unreal, cls_name, None)
    if cls is None:
        unreal.log_warning("make_water: node class {} not in this build".format(cls_name)); return None
    try:
        return mel.create_material_expression(mat, cls, x, y)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("make_water: create {} failed: {}".format(cls_name, e)); return None


def link(a, ap, b, bp):
    if a is None or b is None:
        return False
    try:
        mel.connect_material_expressions(a, ap, b, bp); return True
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("make_water: link ->{} failed: {}".format(bp or "in", e)); return False


def prop(a, ap, mp):
    if a is None:
        return False
    try:
        mel.connect_material_property(a, ap, mp); return True
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("make_water: prop {} failed: {}".format(mp, e)); return False


def scalar(name, val, x, y):
    n = node("MaterialExpressionScalarParameter", x, y); setp(n, "parameter_name", name); setp(n, "default_value", val); return n


def vecp(name, col, x, y):
    n = node("MaterialExpressionVectorParameter", x, y); setp(n, "parameter_name", name); setp(n, "default_value", col); return n


if EAL.does_asset_exist(mat_path):
    EAL.delete_asset(mat_path)
mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
if mat is None:
    raise RuntimeError("make_water: could not create the material at " + mat_path)

setp(mat, "shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
setp(mat, "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
setp(mat, "two_sided", True)
tlm = getattr(unreal.TranslucencyLightingMode, "TLM_SURFACE_PER_PIXEL_LIGHTING", None)
if tlm is not None:
    setp(mat, "translucency_lighting_mode", tlm)

# ── Params ── (clarity-first calm pond; tune live on M_PondWater)
shallow = vecp("ShallowColor", unreal.LinearColor(0.10, 0.45, 0.55, 1.0), -1700, -360)  # per-instance WaterColor overrides this
deep    = vecp("DeepColor",    unreal.LinearColor(0.02, 0.20, 0.34, 1.0), -1700, -260)
edge    = vecp("EdgeColor",    unreal.LinearColor(0.30, 0.50, 0.62, 1.0), -1700, -160)  # soft sky rim — NOT near-white
depthDist = scalar("DepthColorDistance", 500.0, -1700, -60)   # gentle tint ramp so deep water still reads clear
shallowOpac = scalar("ShallowOpacity", 0.12, -1700, 40)       # very clear at the shoreline (see the bottom)
deepOpac    = scalar("DeepOpacity",    0.50, -1700, 120)      # still semi-transparent in deep water
rough   = scalar("Roughness", 0.16, -1700, 200)               # glossy but NOT a mirror (no white sparks)
spec    = scalar("Specular", 0.5, -1700, 280)
foamCol = vecp("FoamColor", unreal.LinearColor(0.9, 0.97, 1.0, 1.0), -1700, 360)
foamW   = scalar("FoamWidth", 35.0, -1700, 440)               # thin shoreline line
foamSharp = scalar("FoamSharpness", 3.0, -1700, 520)
fresPow = scalar("FresnelPower", 5.0, -1700, 600)
fresOpac = scalar("FresnelEdgeOpacity", 0.12, -1700, 680)
# Two GENTLE smooth-gradient-noise ripple fields tilt the normal in X and Y. Smooth + mip-filtered (NOT the
# per-pixel cellular VectorNoise that aliased into TV static), low frequency, slow pan = calm water.
ripScale  = scalar("RippleScale",    0.0025, -1700, 760)
ripSpeed  = scalar("RippleSpeed",    0.40,   -1700, 840)
ripStr    = scalar("RippleStrength", 0.10,   -1700, 920)
ripScale2 = scalar("RippleScale2",   0.0055, -1700, 1000)
ripSpeed2 = scalar("RippleSpeed2",   0.25,   -1700, 1080)
ripStr2   = scalar("RippleStrength2",0.07,   -1700, 1160)
# Refraction: clarity comes from seeing the bottom gently bent (IOR mode).
refrIOR = scalar("RefractionIOR", 1.18, -1700, 1240)

prop(spec, "", unreal.MaterialProperty.MP_SPECULAR)


def noise_field(scaleP, speedP, dx, dy, x, y):
    """One panning SMOOTH gradient-noise field -> a scalar in ~[-1,1]. Uses MaterialExpressionNoise (mip-filtered),
    NOT VectorNoise (cellular, per-pixel, aliases to static)."""
    wp = node("MaterialExpressionWorldPosition", x, y)
    sc = node("MaterialExpressionMultiply", x + 150, y); link(wp, "", sc, "A"); link(scaleP, "", sc, "B")
    tm = node("MaterialExpressionTime", x, y + 110)
    dirv = node("MaterialExpressionConstant3Vector", x, y + 185)
    setp(dirv, "constant", unreal.LinearColor(dx, dy, 0.0, 0.0))
    td = node("MaterialExpressionMultiply", x + 150, y + 140); link(tm, "", td, "A"); link(dirv, "", td, "B")
    tds = node("MaterialExpressionMultiply", x + 300, y + 140); link(td, "", tds, "A"); link(speedP, "", tds, "B")
    pos = node("MaterialExpressionAdd", x + 440, y + 60); link(sc, "", pos, "A"); link(tds, "", pos, "B")
    nz = node("MaterialExpressionNoise", x + 570, y + 60); link(pos or sc, "", nz, "Position")
    fn = (getattr(unreal.NoiseFunction, "NOISEFUNCTION_GRADIENT_TEX", None)
          if hasattr(unreal, "NoiseFunction") else None) \
        or (getattr(unreal.NoiseFunction, "NOISEFUNCTION_SIMPLEX_TEX", None) if hasattr(unreal, "NoiseFunction") else None)
    if fn is not None:
        for _fp in ("noise_function", "function"):
            if setp(nz, _fp, fn):
                break
    setp(nz, "scale", 1.0)  # frequency is set by RippleScale on the world position above
    setp(nz, "quality", 1)
    setp(nz, "levels", 1)
    for _tp in ("turbulence", "b_turbulence"):
        if setp(nz, _tp, False):
            break
    setp(nz, "output_min", -1.0)
    setp(nz, "output_max", 1.0)
    return nz


# ── Water view-thickness: SceneDepth (opaque behind) - PixelDepth (this surface) ──
sd = node("MaterialExpressionSceneDepth", -950, 120)
pd = node("MaterialExpressionPixelDepth", -950, 210)
thick = node("MaterialExpressionSubtract", -820, 150); link(sd, "", thick, "A"); link(pd, "", thick, "B")

# depthAlpha = saturate(thick / DepthColorDistance): 0 at the shoreline -> 1 in deep water.
depth_div = node("MaterialExpressionDivide", -680, -120); link(thick, "", depth_div, "A"); link(depthDist, "", depth_div, "B")
depthAlpha = node("MaterialExpressionSaturate", -560, -120); link(depth_div, "", depthAlpha, "")
dAlpha = depthAlpha or depth_div

# ── Ripple normal: two smooth scalar noise fields tilt the up-normal's X and Y -> normalize ──
nfx = noise_field(ripScale,  ripSpeed,   1.00, 0.20, -1350, 560)
nfy = noise_field(ripScale2, ripSpeed2, -0.25, 1.00, -1350, 980)
nx = node("MaterialExpressionMultiply", -640, 580); link(nfx, "", nx, "A"); link(ripStr,  "", nx, "B")
ny = node("MaterialExpressionMultiply", -640, 980); link(nfy, "", ny, "A"); link(ripStr2, "", ny, "B")
nxy = node("MaterialExpressionAppendVector", -500, 700); link(nx or nfx, "", nxy, "A"); link(ny or nfy, "", nxy, "B")  # (x, y)
oneZ = node("MaterialExpressionConstant", -500, 800); setp(oneZ, "r", 1.0)
nvec = node("MaterialExpressionAppendVector", -360, 720); link(nxy, "", nvec, "A"); link(oneZ, "", nvec, "B")          # (x, y, 1)
nrm = node("MaterialExpressionNormalize", -220, 720); link(nvec or nxy, "", nrm, "")
prop(nrm, "", unreal.MaterialProperty.MP_NORMAL)

# ── Fresnel: grazing-angle rim, fed by the smooth ripple normal; clamped so it can't blow to white ──
fres = node("MaterialExpressionFresnel", -700, -360)
setp(fres, "exponent", 5.0); setp(fres, "base_reflect_fraction", 0.02)
link(fresPow, "", fres, "ExponentIn")  # pin name varies by build; harmless if it doesn't bind (the prop above stands)
link(nrm, "", fres, "Normal")
fres_clamp = node("MaterialExpressionMultiply", -560, -360); setp(fres_clamp, "const_b", 0.6); link(fres, "", fres_clamp, "A")
fres_out = fres_clamp or fres or dAlpha

# ── Foam mask: power(1 - saturate(thick / FoamWidth), FoamSharpness) ──
foam_div = node("MaterialExpressionDivide", -680, 220); link(thick, "", foam_div, "A"); link(foamW, "", foam_div, "B")
foam_sat = node("MaterialExpressionSaturate", -560, 220); link(foam_div, "", foam_sat, "")
foam_inv = node("MaterialExpressionOneMinus", -460, 220); link(foam_sat or foam_div, "", foam_inv, "")
foam = node("MaterialExpressionPower", -360, 220); link(foam_inv, "", foam, "Base"); link(foamSharp, "", foam, "Exponent")
foam_mask = foam or foam_inv

# ── Base color: depth tint -> softly tinted toward EdgeColor by (clamped) Fresnel -> thin foam on top ──
water_col = node("MaterialExpressionLinearInterpolate", -420, -240)
link(shallow, "", water_col, "A"); link(deep, "", water_col, "B"); link(dAlpha, "", water_col, "Alpha")
col_fres = node("MaterialExpressionLinearInterpolate", -280, -300)
link(water_col, "", col_fres, "A"); link(edge, "", col_fres, "B"); link(fres_out, "", col_fres, "Alpha")
bc = node("MaterialExpressionLinearInterpolate", -120, -150)
link(col_fres or water_col, "", bc, "A"); link(foamCol, "", bc, "B"); link(foam_mask, "", bc, "Alpha")
prop(bc or col_fres or water_col, "", unreal.MaterialProperty.MP_BASE_COLOR)

# ── Emissive: just a faint foam glow (NO noise sparkle — that was the TV-static / blown-white culprit) ──
foam_em = node("MaterialExpressionMultiply", -150, 40); link(foamCol, "", foam_em, "A"); link(foam_mask, "", foam_em, "B")
foam_em2 = node("MaterialExpressionMultiply", -30, 40); setp(foam_em2, "const_b", 0.15); link(foam_em, "", foam_em2, "A")
prop(foam_em2 or foam_em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# ── Opacity: very clear shallow -> semi-clear deep, solid at the foam line, small Fresnel lift at grazing ──
op_depth = node("MaterialExpressionLinearInterpolate", -150, 150)
link(shallowOpac, "", op_depth, "A"); link(deepOpac, "", op_depth, "B"); link(dAlpha, "", op_depth, "Alpha")
one = node("MaterialExpressionConstant", -300, 200); setp(one, "r", 1.0)
op_foam = node("MaterialExpressionLinearInterpolate", -30, 150)
link(op_depth, "", op_foam, "A"); link(one, "", op_foam, "B"); link(foam_mask, "", op_foam, "Alpha")
op_lift = node("MaterialExpressionMultiply", -30, 250); link(fres_out, "", op_lift, "A"); link(fresOpac, "", op_lift, "B")
op_add = node("MaterialExpressionAdd", 100, 180); link(op_foam or op_depth, "", op_add, "A"); link(op_lift, "", op_add, "B")
op_final = node("MaterialExpressionSaturate", 220, 180); link(op_add or op_foam, "", op_final, "")
prop(op_final or op_foam or op_depth, "", unreal.MaterialProperty.MP_OPACITY)

# ── Roughness = lerp(Roughness, 0.6, foam) ──
rl = node("MaterialExpressionLinearInterpolate", -150, 340)
rhi = node("MaterialExpressionConstant", -300, 380); setp(rhi, "r", 0.6)
link(rough, "", rl, "A"); link(rhi, "", rl, "B"); link(foam_mask, "", rl, "Alpha")
prop(rl or rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

# ── Refraction: clear, gentle bend of the pond bottom (Index Of Refraction mode; ~1.18 reads clean) ──
_rm = getattr(unreal, "RefractionMode", None)
_ior = getattr(_rm, "RM_INDEX_OF_REFRACTION", None) if _rm else None
if _ior is not None:
    for _rmprop in ("refraction_method", "refraction_mode"):
        if setp(mat, _rmprop, _ior):
            break
prop(refrIOR, "", unreal.MaterialProperty.MP_REFRACTION)

mel.recompile_material(mat)
EAL.save_asset(mat_path)
unreal.log("make_water: built {} (crystal-clear: smooth normal, refraction, no sparkle)".format(mat_path))

# ── Pond collision: make SM_Fishing_Hole solid (complex-as-simple, since the import had no collision) ──
fish = EAL.load_asset("/Game/World/SM_Fishing_Hole.SM_Fishing_Hole")
if fish:
    bs = fish.get_editor_property("body_setup")
    if bs is None:
        unreal.log_warning("make_water: SM_Fishing_Hole has no BodySetup; collision flag not set")
    else:
        cas = getattr(unreal.CollisionTraceFlag, "CTF_USE_COMPLEX_AS_SIMPLE", None)
        if cas is not None and setp(bs, "collision_trace_flag", cas):
            EAL.save_loaded_asset(fish)
            unreal.log("make_water: SM_Fishing_Hole set to complex-as-simple collision")
else:
    unreal.log_warning("make_water: SM_Fishing_Hole not found; skipped collision setup")
