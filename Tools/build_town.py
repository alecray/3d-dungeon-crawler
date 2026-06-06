"""Headless: build the town HUB directly into L_Town as editable placed actors (so it can be tweaked
in-editor), replacing the old runtime code-spawn in ATownGameMode.

Places: SkyAtmosphere + a daytime atmosphere-sun DirectionalLight + a real-time-capture SkyLight +
tuned ExponentialHeightFog (open-sky backdrop); a square ring of boundary walls; a distant vista ring
of silhouette blocks; the hub stations (Shop, Blackjack, Fishing, enter-dungeon Portal, Bonfire); and a
scatter of scenery props. Everything we add goes in the "TownHub" world-outliner folder so it's easy to
select/тweak/delete as a group. Re-runnable: it deletes the previous TownHub-folder actors first, and
skips any station class already placed in the map.

Run AFTER building, with the editor CLOSED:
  UnrealEditor-Cmd.exe <proj> -run=pythonscript -script="Tools/build_town.py" -unattended -nopause -nosplash
"""
import math
import random
import unreal

ELL = unreal.EditorLevelLibrary
FOLDER = "TownHub"
DUNGEON_MAP = "L_DungeonTest"
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")

ELL.load_level("/Game/Maps/Town/L_Town")
actors = ELL.get_all_level_actors()


def cfg(component, prop, value):
    """set_editor_property guarded so one wrong property name doesn't abort the whole build."""
    try:
        component.set_editor_property(prop, value)
    except Exception as e:
        unreal.log_warning("could not set {} = {}: {}".format(prop, value, e))


# ---- Anchor everything to the PlayerStart ----
start = next((a for a in actors if isinstance(a, unreal.PlayerStart)), None)
sl = start.get_actor_location() if start else unreal.Vector(0, 0, 0)
sr = start.get_actor_rotation() if start else unreal.Rotator(0, 0, 0)
fwd = unreal.MathLibrary.get_forward_vector(sr)
right = unreal.MathLibrary.get_right_vector(sr)
floor_z = sl.z - 88.0


def hub_point(fwd_cm, right_cm):
    p = sl + fwd * fwd_cm + right * right_cm
    return unreal.Vector(p.x, p.y, floor_z)


def face_start(loc):
    d = unreal.Vector(sl.x - loc.x, sl.y - loc.y, 0.0)
    return unreal.Rotator(0.0, 0.0, unreal.MathLibrary.conv_vector_to_rotator(d).yaw)


def place(actor, folder=FOLDER, label=None):
    if actor:
        actor.set_folder_path(folder)
        if label:
            actor.set_actor_label(label)
    return actor


# ---- Clean previous run (only our folder) ----
removed = 0
for a in actors:
    try:
        if str(a.get_folder_path()) == FOLDER:
            ELL.destroy_actor(a)
            removed += 1
    except Exception:
        pass
if removed:
    unreal.log("Removed {} previous TownHub actors".format(removed))
actors = ELL.get_all_level_actors()


def has_class(cls):
    return any(isinstance(a, cls) for a in actors)


def spawn_block(loc, rot, size_cm, collide, shadow):
    a = ELL.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    smc = a.static_mesh_component
    smc.set_static_mesh(CUBE)
    a.set_actor_scale3d(unreal.Vector(size_cm.x / 100.0, size_cm.y / 100.0, size_cm.z / 100.0))
    if collide:
        smc.set_collision_profile_name("BlockAll")
        smc.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    else:
        smc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    cfg(smc, "cast_shadow", shadow)
    return place(a)


# ===== 1) Open-sky backdrop =====
if not has_class(unreal.SkyAtmosphere):
    place(ELL.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(sl.x, sl.y, floor_z), unreal.Rotator()), label="SkyAtmosphere")

sun = next((a for a in actors if isinstance(a, unreal.DirectionalLight)), None)
if not sun:
    sun = ELL.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(sl.x, sl.y, floor_z + 1000.0), unreal.Rotator(0.0, -38.0, -35.0))
sun.set_actor_rotation(unreal.Rotator(0.0, -38.0, -35.0), False)
dlc = sun.get_component_by_class(unreal.DirectionalLightComponent)
if dlc:
    cfg(dlc, "atmosphere_sun_light", True)
    cfg(dlc, "intensity", 6.0)
place(sun, label="Sun")

sky = next((a for a in actors if isinstance(a, unreal.SkyLight)), None)
if not sky:
    sky = ELL.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(sl.x, sl.y, floor_z + 300.0), unreal.Rotator())
slc = sky.get_component_by_class(unreal.SkyLightComponent)
if slc:
    cfg(slc, "mobility", unreal.ComponentMobility.MOVABLE)
    cfg(slc, "real_time_capture", True)
    cfg(slc, "source_type", unreal.SkyLightSourceType.SLS_CAPTURED_SCENE)
place(sky, label="SkyLight")

fog = next((a for a in actors if isinstance(a, unreal.ExponentialHeightFog)), None)
if not fog:
    fog = ELL.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(sl.x, sl.y, floor_z - 50.0), unreal.Rotator())
fogc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
if fogc:
    cfg(fogc, "fog_density", 0.015)
    cfg(fogc, "fog_height_falloff", 0.1)
    cfg(fogc, "start_distance", 2500.0)  # fog inscattering color is left default — tweak in-editor
place(fog, label="HeightFog")

# ===== 2) Boundary walls (square ring around the play area) =====
half, wall_h, wall_t = 1600.0, 600.0, 80.0
span = half * 2.0 + wall_t
cz = floor_z + wall_h * 0.5
spawn_block(sl + fwd * half + unreal.Vector(0, 0, cz - sl.z), sr, unreal.Vector(wall_t, span, wall_h), True, False)
spawn_block(sl - fwd * half + unreal.Vector(0, 0, cz - sl.z), sr, unreal.Vector(wall_t, span, wall_h), True, False)
spawn_block(sl + right * half + unreal.Vector(0, 0, cz - sl.z), sr, unreal.Vector(span, wall_t, wall_h), True, False)
spawn_block(sl - right * half + unreal.Vector(0, 0, cz - sl.z), sr, unreal.Vector(span, wall_t, wall_h), True, False)

# ===== 3) Distant vista ring (silhouettes, no collision/shadow) =====
random.seed(1337)
N = 18
for i in range(N):
    ang = (2.0 * math.pi * i) / N + random.uniform(-0.06, 0.06)
    radius = random.uniform(6500.0, 9500.0)
    h = random.uniform(1400.0, 4200.0)
    w = random.uniform(900.0, 2400.0)
    loc = unreal.Vector(sl.x + math.cos(ang) * radius, sl.y + math.sin(ang) * radius, floor_z + h * 0.5)
    rot = unreal.Rotator(0.0, 0.0, math.degrees(ang) + random.uniform(-20.0, 20.0))
    spawn_block(loc, rot, unreal.Vector(w, w * random.uniform(0.6, 1.2), h), False, False)

# ===== 4) Hub stations (skip any class already placed) =====
def station(cls, fwd_cm, right_cm, label):
    if has_class(cls):
        unreal.log("{} already placed — skipping".format(label))
        return None
    loc = hub_point(fwd_cm, right_cm)
    return place(ELL.spawn_actor_from_class(cls, loc, face_start(loc)), label=label)

portal = station(unreal.Portal, 750.0, 0.0, "EnterDungeonPortal")
if portal:
    cfg(portal, "target_map_name", DUNGEON_MAP)
    cfg(portal, "b_active", True)
station(unreal.ShopNPC, 420.0, -620.0, "Shop")
station(unreal.BlackjackTable, 420.0, 620.0, "Blackjack")
station(unreal.FishingHole, -350.0, 700.0, "FishingHole")
station(unreal.Bonfire, 250.0, 0.0, "Bonfire")

# ===== 5) Scenery scatter =====
scatter = [unreal.PropType.BARREL, unreal.PropType.CRATE, unreal.PropType.POTS,
           unreal.PropType.BONES, unreal.PropType.BUCKET, unreal.PropType.ROCKS]
random.seed(99)
for i in range(10):
    fc = random.uniform(-1100.0, 1100.0)
    rc = random.uniform(-1100.0, 1100.0)
    if abs(fc) < 250.0 and abs(rc) < 250.0:
        continue
    loc = hub_point(fc, rc)
    rot = unreal.Rotator(0.0, 0.0, random.uniform(0.0, 360.0))
    prop = ELL.spawn_actor_from_class(unreal.DungeonProp, loc, rot)
    if prop:
        cfg(prop, "prop_type", scatter[random.randrange(len(scatter))])
        place(prop)

ELL.save_current_level()
unreal.log("build_town: saved L_Town with the hub built into the level")
